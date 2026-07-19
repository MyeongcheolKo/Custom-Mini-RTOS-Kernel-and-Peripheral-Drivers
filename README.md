# Custom-Mini-RTOS

A mini RTOS kernel for ARM Cortex-M built from scratch, targeting STM32F446xx. The kernel core is portable C with the architecture-specific code isolated in a port layer, and the project also includes bare-metal infrastructure (linker script, startup code) and a peripheral driver library.

## Table of Contents
- [Project Structure](#project-structure)
- [Kernel](#kernel)
- [Peripheral Drivers](#peripheral-drivers)
  - [GPIO Driver](#gpio-driver)
  - [SPI Driver](#spi-driver)
  - [I2C Driver](#i2c-driver)
  - [USART Driver](#usart-driver)
  - [Sample Applications](#sample-applications)
- [Bare-Metal Infrastructure](#bare-metal-infrastructure)
  - [Memory Map](#memory-map)
  - [Linker Script](#linker-script)
  - [Startup Code](#startup-code)
  - [Build System (Makefile)](#build-system-makefile)
- [See It In Action!](#see-it-in-action)
- [Ideas for Future Improvements](#ideas-for-future-improvements)

# Project Structure

The project is organized into layers, each buildable on its own:

```
.
├── kernel/                    # portable RTOS core (pure C, no arch code)
│   ├── kernel.c               # kernel core: scheduling, tasks, tick, delay
│   ├── os.h                   # public API — apps include this
│   └── kernel_internal.h      # core <-> port interface
├── port/arm/cortex_m4/        # architecture port (all ARM asm + register access)
│   └── port.c/.h              # context switch, SysTick/PendSV setup, critical sections
├── config/
│   └── osConfig.h             # compile-time kernel config (tick rate, stack sizes, ...)
├── drivers/                   # STM32F446xx peripheral HAL (GPIO, SPI, I2C, USART, RCC)
├── bsp/                       # board bring-up: startup.c, linker_script.ld, syscalls
├── sample_apps/
│   ├── kernel/                # RTOS demos (round_robin_priority.c, ...)
│   └── driver/                # peripheral demos (LED_toggle.c, SPI_testing.c, ...)
└── makefile
```

The **kernel** and **drivers** build into independent static libraries (`librtos.a`, `libdrivers.a`) with no dependency on each other — the kernel compiles with zero driver code, and vice versa. Only the `port/` layer is Cortex-M4 specific; the kernel core is portable C.

# Kernel

A preemptive, priority-based scheduler that manages user tasks plus an idle task, using hardware features of the ARM Cortex-M architecture:

- **SysTick Timer** — Generates periodic interrupts (1ms ticks) to drive scheduling
- **PendSV Exception** — Performs the actual context switch at the lowest priority
- **Dual Stack Pointers** — MSP for kernel/handlers, PSP for user tasks
- **Priority scheduling** — On each tick the highest-priority `READY` task is selected (lower priority number = higher priority; `0` reserved for the idle task)

The kernel is split into a **portable core** (`kernel/` — scheduling logic, TCB bookkeeping, blocking/tick handling; pure C) and an **architecture port** (`port/arm/cortex_m4/` — context switch, SysTick/PendSV setup, critical sections; all ARM asm and register access). Applications include a single kernel header — `os.h` — which exposes all public-facing APIs (`kernel_internal.h`, the core↔port glue stays in a separate header and user should not include it). Only the `port` needs rewriting to target a different architecture.
```
┌─────────────────────────────────────────────────────────┐
│                      SRAM Layout                        │
├─────────────────────────────────────────────────────────┤
│  Scheduler Stack (MSP)    <- Used by exception handlers │
├─────────────────────────────────────────────────────────┤
│  Task 1 Private Stack (PSP)                             │
├─────────────────────────────────────────────────────────┤
│  Task 2 Private Stack (PSP)                             │
├─────────────────────────────────────────────────────────┤
│  Task 3 Private Stack (PSP)                             │
├─────────────────────────────────────────────────────────┤
│  Task 4 Private Stack (PSP)                             │
├─────────────────────────────────────────────────────────┤
│  Idle Task Stack (PSP)                                  │
└─────────────────────────────────────────────────────────┘
How It Works: 
    ┌──────────┐  os_task_delay()  ┌─────────┐
    │  READY   │ ----------------> │ BLOCKED │
    └──────────┘                   └─────────┘
         |                             |
         |     wakeup_tick reached     |
         └──────<───────<────────<─────┘
```

### Task Lifecycle

- All tasks start in `READY` state
- When a task calls `os_task_delay(ticks)`, it transitions to `BLOCKED`
- The scheduler skips blocked tasks and selects the highest-priority `READY` task
- Global `systick_count` gets updated at every `SysTick_Handler`
- When `systick_count` reaches the task's `wakeup_tick`, it becomes `READY` again

### Context Switch Flow
```
SysTick fires (every 1ms)
           |
  ┌───────────────────┐
  │ Update tick count │
  │   Unblock tasks   │
  │   Pend PendSV     │ <- Doesn't switch here, just sets pending bit
  └───────────────────┘
           |        (after all higher-priority interrupts complete)
┌──────────────────────┐
│        PendSV        │
│ Save R4-R11 manually │ <- Hardware auto-saves R0-R3, R12, LR, PC, xPSR
│   Select next task   │ 
│    Restore R4-R11    │
└──────────────────────┘
```

## Design Choices

- **Dummy stack frame** — Created so the first context switch works. When a task runs for the first time, there's no "previous context" to retrieve, so we initialize the stack with a fake frame.

- **Blocking state + SysTick timer** — The delay time for software delay is actually (task delay + delays of other tasks). For example, if Task1 wants a 1000ms delay but Task2-4 also have delays of
  500ms, 250ms, and 2000ms: Task1's real wait time = 1000 + 500 + 250 + 2000 = 3750ms. By adding a blocking state and using the SysTick timer, a blocked task is skipped during scheduling and the scheduler
  immediately moves to the next ready task. This way each task's delay is independent of what other tasks are doing.

- **Idle task** — Always `READY` and never blocks, so `os_schedule_next_task()` always has a valid task to select when all other tasks are blocked.

- **PendSV for context switching** — Chose PendSV instead of doing it in SysTick because PendSV has lower priority, so it only gets executed after all other interrupts. This way we won't exit an interrupt handler due to a context switch, which causes a usage fault. We only pend the PendSV and do the context switch when all other interrupts and exceptions are dealt with.

- **Naked functions** — Used for `port_switch_to_psp`, `PendSV_Handler`, and `port_init_scheduler_stack` (all in the port layer) to deal with the prologue and epilogue of C functions corrupting LR:
  - `port_switch_to_psp`: Prologue would push LR to the old stack (MSP), then epilogue would pop from the new stack (PSP) -> corruption
  - `PendSV_Handler`: Need manual control over what gets pushed/popped to the stack and where for context switching + function calls corrupt the EXC_RETURN value in LR
  - `port_init_scheduler_stack`: Modifying MSP itself, prologue/epilogue would use old/new MSP inconsistently
- **Race condition in `os_task_delay`** - Disabled interrupts while setting `wakeup_tick` and `current_state` to prevent a race condition with `SysTick_Handler`. Without this, SysTick could update `systick_count` between reading it and setting the blocked state, corrupting the task's wake-up deadline. `unblock_tasks` compares the signed difference (`systick_count - wakeup_tick >= 0`) so it also stays correct when the tick counter wraps around.

# Peripheral Drivers

The peripheral driver library provides a hardware abstraction layer (HAL) for STM32F446xx peripherals. Each driver follows a consistent API pattern with configuration structures, handle structures, and consistent function naming. It also includes sample applications to test/demonstrate the use of the drivers.

### Driver Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                      Application Layer                          │
├─────────────────────────────────────────────────────────────────┤
│ GPIO_driver   │   SPI_driver   │   I2C_driver   │  USART_driver │
├─────────────────────────────────────────────────────────────────┤
│                         STM32F446xx.h                           │
└─────────────────────────────────────────────────────────────────┘
```

### Common Driver Features

All drivers share these characteristics:
- **Clock Control** — Enable/disable peripheral clocks via RCC
- **Initialization** — Configure peripheral with user-specified settings
- **De-initialization** — Reset peripheral registers to default state
- **Send and Receive Data** — Both polling and interrupt-driven modes
- **Weak Callbacks** — Overrideable callback functions for interrupt events

### STM32F446xx.h

## GPIO Driver

General Purpose Input/Output driver supporting all GPIO ports (A-H).

#### Features
- Pin mode configuration (Input, Output, Alternate Function, Analog)
- Output type (Push-Pull, Open-Drain)
- Speed configuration (Low, Medium, Fast, High)
- Pull-up/Pull-down resistor configuration
- Interrupt support (Rising edge, Falling edge, Both edges)
- Alternate function selection 

## SPI Driver

Serial Peripheral Interface driver supporting SPI1-SPI4.

#### Features
- Master/Slave mode configuration
- Full-duplex, Half-duplex, and Simplex communication
- Configurable clock speed (prescaler from /2 to /256)
- 8-bit or 16-bit data frame format
- Clock polarity (CPOL) and phase (CPHA) configuration
- Hardware/Software slave select management
- Interrupt-driven transmit and receive
- Overrun error handling

## I2C Driver

Inter-Integrated Circuit driver supporting I2C1-I2C3.

#### Features
- Controller (Master) and Target (Slave) mode
- Standard mode (100kHz) and Fast mode (200kHz, 400kHz)
- 7-bit addressing
- Repeated start condition support
- ACK/NACK management
- Interrupt-driven communication
- Comprehensive error handling (Bus error, Arbitration loss, ACK failure, Overrun, Timeout)

## USART Driver

Universal Synchronous/Asynchronous Receiver-Transmitter driver supporting USART1-3, UART4-5, USART6.

#### Features
- Transmit-only, Receive-only, or Full-duplex mode
- Wide range of baud rates (1200 to 3M)
- 8-bit or 9-bit word length
- Configurable parity (None, Even, Odd)
- Stop bit configuration (0.5, 1, 1.5, 2 bits)
- Hardware flow control (CTS, RTS)
- Interrupt-driven communication
- Error detection (Framing, Noise, Overrun)

## Sample Applications

The `sample_apps/driver/` directory contains working driver examples (kernel demos live in `sample_apps/kernel/`, e.g. `round_robin_priority.c`). Build any of them with `make APP=<path>` (see [Build System](#build-system-makefile)):

| Application (`sample_apps/driver/`) | Description |
|-------------|-------------|
| `LED_toggle.c` | Basic GPIO output - toggles onboard LED |
| `button_LED.c` | GPIO input/output - LED controlled by button press |
| `external_button_LED.c` | External button and LED with pull-up configuration |
| `interrupt_button_LED.c` | GPIO interrupt - LED toggle on button interrupt |
| `SPI_testing.c` | SPI loopback test |
| `SPI_transmit_arduino.c` | SPI master transmit to Arduino slave |
| `SPI_send_recieve_arduino.c` | SPI bidirectional communication with Arduino |
| `I2C_controller_send_arduino.c` | I2C master send to Arduino slave |
| `I2C_controller_send_receive.c` | I2C master send/receive with Arduino |
| `I2C_interrupt_send_receive.c` | I2C interrupt-driven communication |


# Bare-Metal Infrastructure
The bare-metal infrastructure includes everything needed to get code running on the MCU before the scheduler takes over, including the linker script, startup code, debug output configuration, and the build system.

## Memory Map

```


┌─────────────────────────────────────┐ (SRAM_END) (T1_STACK_START)
│         Task 1 Stack (PSP)          │
│              (1 KB)                 │
├─────────────────────────────────────┤ (T2_STACK_START)
│         Task 2 Stack (PSP)          │
│              (1 KB)                 │
├─────────────────────────────────────┤ (T3_STACK_START)
│         Task 3 Stack (PSP)          │
│              (1 KB)                 │
├─────────────────────────────────────┤ (T4_STACK_START)
│         Task 4 Stack (PSP)          │
│              (1 KB)                 │
├─────────────────────────────────────┤ (IDLE_STACK_START)
│        Idle Task Stack (PSP)        │
│              (1 KB)                 │
├─────────────────────────────────────┤ (SCHEDU_STACK_START)
│         MSP Stack (Kernel)          │
│              (1 KB)                 │
├─────────────────────────────────────┤
│                                     │
│          Available SRAM             │
│        (Heap grows upward)          │
│                                     │
├─────────────────────────────────────┤ 
│              .bss                   │
├─────────────────────────────────────┤ 
│              .data                  │
└─────────────────────────────────────┘ 0x20000000 (SRAM_START)


┌─────────────────────────────────────┐ 
│                                     │
│           Available FLASH           │
│                                     │
├─────────────────────────────────────┤
│             .rodata                 │
├─────────────────────────────────────┤
│              .text                  │
├─────────────────────────────────────┤ 
│            .isr_vector              │
│          (Vector Table)             │
└─────────────────────────────────────┘ 0x08000000 (FLASH_START)
```

## Linker Script

The linker script defines how the compiled code is organized in FLASH and SRAM.

| Region | Start Address | Size | Attributes |
|--------|--------------|------|------------|
| FLASH  | 0x08000000   | 512K | rx (read, execute) |
| SRAM   | 0x20000000   | 128K | rwx (read, write, execute) |

### Sections:

| Section | Location | Description |
|---------|----------|-------------|
| `.isr_vector` | FLASH | Interrupt vector table (must be at 0x08000000) |
| `.text` | FLASH | Executable code |
| `.rodata` | FLASH | Read-only data (constants, strings) |
| `.data` | SRAM (VMA), FLASH (LMA) | Initialized global/static variables |
| `.bss` | SRAM | Uninitialized global/static variables (zeroed at startup) |


### .data Section 

The `.data` section is special because it is stored in FLASH (LMA) since RAM is volatile but is then copied to SRAM (VMA) by startup code before `main()` executes and resides in SRAM at runtime

### Linker Symbols

```c
_estack      // Top of stack (end of SRAM)
_sdata       // Start of .data in SRAM (VMA)
_edata       // End of .data in SRAM
_sidata      // Start of .data in FLASH (LMA) - initialization source
_sbss        // Start of .bss
_ebss        // End of .bss
_end         // End of used SRAM (heap starts here)
_Min_Stack_Size  // Reserved stack space (0x400 = 1KB)
```
- `. = ALIGN(4)` - used at the start and end of each section to force 4-byte alignment, which ensures word-aligned access and proper copying in startup code (which copies 4 bytes at a time).
- `_sidata = LOADADDR(.data)` - to ensure proper copying of .data section from FLASH to SRAM in startup code.

## Startup Code

The startup code runs before `main()` and initializes *.data* and *.bss* in SRAM.

### Vector Table

The vector table is implemented according to the STM32F446xx Reference Manual, including all the **system exception handlers**, **IRQ handlers**, and **reserved** entries.

### Reset Handler Sequence

```
        Power On / Reset
               |
               |
  ┌──────────────────────────┐
  │   1. Copy .data section  │  
  │    from FLASH to SRAM    │
  └──────────────────────────┘
               |
               |
  ┌──────────────────────────┐
  │    2. Fill .bss with 0   │
  └──────────────────────────┘
               |
┌───────────────────────────────┐
│  3. Call __libc_init_array()  │  
└───────────────────────────────┘
               |
               |
  ┌──────────────────────────┐
  │     4. Call main()       │
  └──────────────────────────┘
```

### Weak Aliases

All interrupt handlers are declared as `__attribute__(( weak, alias ("Default_Handler")))`. This makes all exception handlers point to the `Default_Handler`,which is just a infinite `while(1)` loop, by default to handle all the interrupts that I didn't use. The `weak` attribute allows me to override the handlers to the actual implmentation I have. 

## Build System (Makefile)

A plain `make` build (no IDE required). The kernel and drivers compile into independent static libraries, which link with a selected sample application into one firmware image under `build/`.

### Configurations: 

- `arm-none-eabi-gcc`: to cross compiler for ARM 
- `-mcpu = cortex-m4`: to specify the target processor for my board (STM32F446RE)
- `-mthumb`: generate Thumb ISA rather than ARM, due to cortex-m4 processors only running Thumb
- `-mfloat-abi=soft`: use software floating-point, I didn't have any float-point calculation in my code so I didn't both with setting up the FPU and no FPU library overhead 

### Build Commands

| Command | Description |
|---------|-------------|
| `make` | build the full firmware (`build/final.elf`) |
| `make kernel` | build only the kernel library (`librtos.a`) — proves it compiles with zero driver dependencies |
| `make drivers` | build only the driver library (`libdrivers.a`) |
| `make clean` | remove the `build/` directory |
| `make load` | flash via OpenOCD (use `arm-none-eabi-gdb` to talk to the OpenOCD server) |

### Selecting the Application

The default app is the kernel round-robin demo. Build any other sample by overriding `APP`:

```
make                                        # sample_apps/kernel/round_robin_priority.c (default)
make APP=sample_apps/driver/LED_toggle.c    # a driver sample
```

The selected app's own directory is added to the include path automatically.

### Static Libraries

| Artifact | Built from | Notes |
|----------|-----------|-------|
| `build/librtos.a` | `kernel/` + `port/` | the RTOS kernel as a standalone library |
| `build/libdrivers.a` | `drivers/` | the peripheral HAL as a standalone library |

`librtos.a` is force-linked with `--whole-archive` because it holds the `PendSV`/`SysTick` handlers that `startup.c` only weak-aliases (otherwise the linker keeps the weak stubs and context switching silently dies). `libdrivers.a` links on demand — only the drivers the selected app references. The build uses `nano.specs` (Newlib-nano) for a small footprint.

### Output Files

All build artifacts go under `build/` (git-ignored), mirroring the source tree:

| File | Description |
|------|-------------|
| `build/final.elf` | the flashable firmware |
| `build/librtos.a` / `build/libdrivers.a` | kernel / driver static libraries |
| `build/final.map` | linker map (symbol addresses, section sizes) |

## Debugging

### ITM Debug Output

Printf output is routed to SWV ITM Data Console Port 0 in `syscalls.c` for debugging in the STM32CudeIDE. This is the defualt `make all` command. 

- This was used in the STM32CudeIDE for debugging when implementing the scheduler since ITM is non-blocking so it doesn't interfere with task timing. 

> Semihosting (`rdimon.specs`) was used during early bring-up to get `printf` over the debugger before syscalls were implemented; it has since been removed in favor of the ITM path above.

## See It In Action!
Watch the kernel and peripheral driver sample applications in action: [YouTube Playlist](https://www.youtube.com/playlist?list=PLLaVu9P3il1isXzX8xk3gsnbkaftZR-8b)



## Ideas for Future Improvements
- Synchronization primitives — semaphores, mutexes, message queues (each as its own kernel sample app)
- Dynamic task creation/deletion
- Additional ports (Cortex-M0, RISC-V) to exercise the port layer
- Add stack canaries or MPU protection