#include <stdint.h>
#include <stdio.h>

#include "os.h"

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

/*
round_robin_priority.c

Demonstrates the kernel's two scheduling policies at once:
  - PRIORITY based selection: the highest-priority READY task always runs.
  - ROUND-ROBIN: tasks of equal priority take turns, none starves.

Tasks (created in main; lower number = higher priority, 0 is the idle task):
  task1, priority 2, prints, then busy_spin()          -> CPU-bound, never blocks
  task2, priority 2, prints, then busy_spin()          -> CPU-bound, never blocks
  task3, priority 1, prints, then os_task_delay(1000)  -> highest priority, sleeps
  task4, priority 2, prints, then busy_spin()          -> CPU-bound, never blocks

Tasks 1/2/4 use busy_spin() (a plain CPU loop) instead of os_task_delay on
purpose: a delayed task becomes BLOCKED and leaves the ready set, which would
hide round-robin. Spinning keeps them READY so the scheduler must rotate between 
them. Task 3 uses os_task_delay to demonstrate that it preempts the other tasks 
when it wakes, proving priority.

Expected ITM/SWO output (repeats):
Task schedular initialized
This is task 3    <- highest priority runs first             [PRIORITY]
This is task 4    >
This is task 1    >  equal-priority tasks rotate, none starves [ROUND-ROBIN]
This is task 2    >
This is task 3    <- task3 wakes from its delay and preempts [PRIORITY]
This is task 4
This is task 1
This is task 2
  ...

Reading it:
- task3 appearing first, and cutting back in every time it wakes, proves
priority: a priority 1 task always beats the priority 2 tasks when READY.
- task1, task2 AND task4 all appearing proves round-robin: three equal-
priority tasks all get the CPU, and task 4 is scheduled first then task 1 and task 2.

NOTE: printf is neither atomic nor reentrant, and the kernel is preemptive, so a
context switch can land in the middle of a printf. When it does, a second task's
output gets braided into the first's, and because the two calls share one global
(non-reentrant) printf buffer, some characters are also lost:

    task1 prints:  "This is task 1"
    task2 prints:  "This is task 2 "
    ITM may show:  "This is taskThis is ta 2 sk 2"   (interleaved + chars dropped)

This is a printf limitation, not a scheduler bug. Serialize printf (critical
section, mutex, or a single printer task) for clean output.
 */

#define SPIN_BETWEEN_PRINTS 500000u   // bigger = slower prints; tune for a readable ITM rate

// enables UsageFault, BusFault, and MemManageFault so they trap as their own exceptions instead of escalating to HardFault
void enable_processor_faults(void);

void error_hanlder(void);

void task1_handler(void);
void task2_handler(void);
void task3_handler(void);
void task4_handler(void);

void busy_spin(uint32_t iterations);

uint32_t task1_stack[1024] __attribute__((aligned(8)));
uint32_t task2_stack[1024] __attribute__((aligned(8)));
uint32_t task3_stack[1024] __attribute__((aligned(8)));
uint32_t task4_stack[1024] __attribute__((aligned(8)));

int main(void)
{
	// keep the debugger awake when the core is sleeping, so can still debug while the core is sleeping
	*(volatile uint32_t *)0xE0042004 |= (1 << 0); // STM32F4: DBGMCU_CR @ 0xE0042004, bit0 DBG_SLEEP, bit1 DBG_STOP, bit2 DBG_STANDBY

 	enable_processor_faults();

	if (os_task_create(task1_handler, 2, task1_stack, sizeof(task1_stack)) != OS_OK) error_hanlder();
	if (os_task_create(task2_handler, 2, task2_stack, sizeof(task2_stack)) != OS_OK) error_hanlder();
	if (os_task_create(task3_handler, 1, task3_stack, sizeof(task3_stack)) != OS_OK) error_hanlder();
	if (os_task_create(task4_handler, 2, task4_stack, sizeof(task4_stack)) != OS_OK) error_hanlder();

	printf("Task schedular initialized\n");

	os_kernel_start();

	while (1)
	{
		// should never get here
	}
}

// burns CPU WITHOUT blocking, so the caller stays READY (this is the key difference from os_task_delay)
void busy_spin(uint32_t iterations)
{
    for (volatile uint32_t k = 0; k < iterations; k++) { }
}

void task1_handler(void)
{
	while(1)
	{
		printf("This is task 1\n");
		// slow down prints so they are readable in ITM viewer, but the task is still running and not blocked
		busy_spin(SPIN_BETWEEN_PRINTS); 
	}
}
void task2_handler(void)
{
	while(1)
	{
		printf("This is task 2 \n");
		// slow down prints so they are readable in ITM viewer, but the task is still running and not blocked
		busy_spin(SPIN_BETWEEN_PRINTS); 
	}
}
void task3_handler(void)
{
	while(1)
	{
		printf("This is task 3\n");
		os_task_delay(1000);
	}
}
void task4_handler(void)
{
	while(1)
	{
		printf("This is task 4\n");
		// slow down prints so they are readable in ITM viewer, but the task is still running and not blocked
		busy_spin(SPIN_BETWEEN_PRINTS);
	}
}

void error_hanlder(void)
{
	while(1);
}

void enable_processor_faults(void)
{
	uint32_t *p_SHCSR = (uint32_t *)0xE000ED24;
	*p_SHCSR |= (1 << 18); // usage fault
	*p_SHCSR |= (1 << 17); // bus fault
	*p_SHCSR |= (1 << 16); // mem fault
}

void HardFault_Handler(void)
{
	printf("hard fault\n");
	while(1);
}

void MemManage_Handler(void)
{
	printf("mem fault\n");
	while(1);
}

void BusFault_Handler(void)
{
	printf("bus fault\n");
	while(1);
}

void UsageFault_Handler(void)
{
	printf("usage fault\n");
	while(1);
}
