CC   = arm-none-eabi-gcc
AR   = arm-none-eabi-ar
MACH = cortex-m4
BUILD_DIR = build

# change this to the sample application you want to build, e.g. ` make APP=sample_applications/driver/LED_toggle.c`
APP ?= sample_apps/kernel/round_robin_priority.c

# header search paths (the selected app's own dir is added so it finds its headers)
INCLUDES = -I$(dir $(APP)) -Ibsp -Iconfig -Ikernel -Iport/arm/cortex_m4 -Idrivers

CFLAGS     = -c -mcpu=$(MACH) -mthumb -mfloat-abi=soft -std=gnu11 -Wall -O0 -g $(INCLUDES)
LDFLAGS    = -mcpu=$(MACH) -mthumb -mfloat-abi=soft --specs=nano.specs  -T bsp/linker_script.ld -Wl,-Map=$(BUILD_DIR)/final.map

# source groups (one per layer) 
KERNEL_SRCS = kernel/kernel.c \
              port/arm/cortex_m4/port.c
DRIVER_SRCS = drivers/GPIO_driver.c \
              drivers/I2C_driver.c \
              drivers/SPI_driver.c \
              drivers/USART_driver.c \
              drivers/rcc.c
APP_SRCS    = $(APP)
BSP_SRCS    = bsp/startup.c \
              bsp/syscalls.c \
              bsp/sysmem.c

# src/main.c -> build/src/main.o (mirrors the tree)
KERNEL_OBJS = $(addprefix $(BUILD_DIR)/,$(KERNEL_SRCS:.c=.o))
DRIVER_OBJS = $(addprefix $(BUILD_DIR)/,$(DRIVER_SRCS:.c=.o))
APP_OBJS    = $(addprefix $(BUILD_DIR)/,$(APP_SRCS:.c=.o))
BSP_OBJS    = $(addprefix $(BUILD_DIR)/,$(BSP_SRCS:.c=.o))

LIBRTOS    = $(BUILD_DIR)/librtos.a
LIBDRIVERS = $(BUILD_DIR)/libdrivers.a

# librtos.a holds the ISRs (PendSV_Handler / SysTick_Handler). startup.c only
# weak-aliases those to Default_Handler, so unless the whole archive is force-linked
# the linker can keep the weak stubs and context switching silently dies. Pull the
# whole kernel archive so the strong handlers win. libdrivers.a has no vector-table
# handlers, so it links on demand (only what the app actually references).
WHOLE_RTOS = -Wl,--whole-archive $(LIBRTOS) -Wl,--no-whole-archive

# full integrated firmware (app + bsp + kernel + drivers)
all: $(BUILD_DIR)/final.elf

# build a layer on its own (kernel has zero driver dependencies, and vice versa)
kernel:  $(LIBRTOS)
drivers: $(LIBDRIVERS)

# compile: any .c -> build/.../.o, creating mirrored subdirs
$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

# static libraries
$(LIBRTOS): $(KERNEL_OBJS)
	$(AR) rcs $@ $^

$(LIBDRIVERS): $(DRIVER_OBJS)
	$(AR) rcs $@ $^

# link: app + bsp objects against both libs
$(BUILD_DIR)/final.elf: $(APP_OBJS) $(BSP_OBJS) $(LIBRTOS) $(LIBDRIVERS)
	$(CC) $(LDFLAGS) $(APP_OBJS) $(BSP_OBJS) $(WHOLE_RTOS) $(LIBDRIVERS) -o $@

clean:
	rm -rf $(BUILD_DIR)

load:
	openocd -f board/st_nucleo_f4.cfg
