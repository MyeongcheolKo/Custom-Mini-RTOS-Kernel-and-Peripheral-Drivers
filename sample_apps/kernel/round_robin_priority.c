#include <stdint.h>
#include <stdio.h>

#include "os.h"

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

// enables UsageFault, BusFault, and MemManageFault so they trap as their own exceptions instead of escalating to HardFault
void enable_processor_faults(void);

void error_hanlder(void);

void task1_handler(void);
void task2_handler(void);
void task3_handler(void);
void task4_handler(void);

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

void task1_handler(void)
{
	//the tasks never returns(finishes)
	while(1)
	{
		printf("This is task 1\n");
		os_task_delay(250);
	}
}
void task2_handler(void)
{
	while(1)
	{
		printf("This is task 2 \n");
		os_task_delay(249);

	}
}
void task3_handler(void)
{
	while(1)
	{
		printf("This is task 3\n");
		os_task_delay(500);

	}
}
void task4_handler(void)
{
	while(1)
	{
		printf("This is task 4\n");
		os_task_delay(2000);
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
