/*
 * scheduler.c
 *
 *  Created on: Dec 23, 2025
 *      Author: krisko
 */
#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "scheduler.h"
#include "tasks.h"

uint32_t current_task = 0; //start with idle task
TCB_t user_tasks[MAX_TASKS];
uint32_t systick_count = 0;

static void save_sp_value(uint32_t current_psp_val);
static uint32_t get_sp_value(void);
static void update_tick_count(void);
static void pend_pendsv(void);
static void unblock_tasks(void);
static void update_next_task(void);

// configures SysTick to fire at tick_hz interrupts per second using the processor clock
void os_init_systick_timer(uint32_t tick_hz)
{
	uint32_t count_val = (SYSTICK_CLOCK / tick_hz) - 1;
	uint32_t *p_SYST_RVR = (uint32_t*)0xE000E014; // systick reload value register

	// clear the value 
	*p_SYST_RVR &= ~(0x00FFFFFF);

	// load the value that will be reloaded into the register when it reaches 0
	// achieve the desired systick freq by configuring this
	*p_SYST_RVR |= count_val;

	uint32_t *p_SYST_CSR = (uint32_t*)0xE000E010; //  SysTick Control and Status Register
	*p_SYST_CSR |= (1 << 1); // enable the systick exception request
	*p_SYST_CSR |= (1 << 2); // use processor clock
	*p_SYST_CSR |= 1; // enable the counter
}

// sets MSP to the top of the scheduler stack before any C stack activity occurs
__attribute__ ((naked)) void os_init_scheduler_stack(uint32_t scheduler_top_of_stack)
{
	/*
	naked func here bc MSR MSP writes to the stack before a valid stack exists, so a compiler generated
	prologue that pushes to MSP would corrupt memory before the stack is set up

	naked forces compiler to not generate prologue and epilogues
	*/

	// change msp (handler sp) to appropriate address of the scheduler stack
	__asm volatile("MSR MSP,%0" : : "r"(scheduler_top_of_stack));	// MSR(move to special register)
	// return to the caller func,
	__asm volatile("BX LR"); // BX (branch and exchange to a adress) LR(address of the caller)
}

// builds a dummy exception stack frame for each task so PendSV can restore it on first run
void os_init_task_stack(void)
{
	/*
		when first time the tasks are run, there were no past context
		so no context to retrieve as there are nothing on the stack
		make dummy stack frames:
		general registers -> all set to 0
		xPSR -> only need t bit to be 1
		PC -> the corresponding task handler
		LR -> EXC_RETURN, should be 0xFFFFFFFD as we need return to thread with PSP
	*/
	user_tasks[0].stack_pointer = IDLE_STACK_START;
	user_tasks[1].stack_pointer = T1_STACK_START;
	user_tasks[2].stack_pointer = T2_STACK_START;
	user_tasks[3].stack_pointer = T3_STACK_START;
	user_tasks[4].stack_pointer = T4_STACK_START;

	user_tasks[0].task_handler = idle_task;
	user_tasks[1].task_handler = task1_handler;
	user_tasks[2].task_handler = task2_handler;
	user_tasks[3].task_handler = task3_handler;
	user_tasks[4].task_handler = task4_handler;

	// create dummy stack frames for all tasks
	uint32_t *p_stack;
	for (int i = 0; i < MAX_TASKS; i++)
	{
		p_stack = (uint32_t*) user_tasks[i].stack_pointer;

		// Configure xPSR(program status register)
		p_stack--;	//stack model is full(always points to last occupied address) descending(grows from higher memory address to lower) so decrement first, then wrie values
		*p_stack = DUMMY_XPSR;	

		/*
		PC(program counter), should hold address of the next instruction to execute
		here direct to the corresponding task_handler for each task
		*/
		p_stack--;
		*p_stack = (uint32_t) user_tasks[i].task_handler;

		/*
		LR(link register), should normally hold address to the next instruction from where this func is called
		but here use exception return with thread mode and psp stack pointer, when entering an exception
		LR will be set to an EXC_RETURN value and LR will be loaded into PC to signal the exception is complete
		Context switching happens in PendSV, which is an exception.
		Hardware will set LR to an EXC_RETURN at entry of an exception but keeping the value consistent here makes sens
		as we need smth to sarisfy the stack frame layout
		*/
		p_stack--;
		*p_stack = EXC_RETURN_THREAD_PSP;

		// zero out all general registers
		for (int j = 0; j < 13; j++)
		{
			p_stack--;
			*p_stack = 0;
		}
		// preserve the value of PSP, as it is modified during the process
		user_tasks[i].stack_pointer = (uint32_t)p_stack;
		// initialize all tasks to be ready state
		user_tasks[i].current_state = TASK_READY_STATE;
	}
}

// saves the current task's PSP into its TCB
static void os_save_sp_value(uint32_t current_psp_val)
{
	user_tasks[current_task].stack_pointer = current_psp_val;
}

// returns the saved PSP of the current task from its TCB
static uint32_t os_get_sp_value(void)
{
	return user_tasks[current_task].stack_pointer;
}

// switches the active stack pointer from MSP to PSP so tasks run on their own private stacks
__attribute__((naked)) void os_switch_to_psp(void)  
{
	/*
	naked func here bc we are calling BL, which corrupts LR, and MSR CONTROL to switch the active
	stack pointer. A compiler prologue/epilogue would push/pop using the wrong stack after the
	stack pointer switch
	*/

	/* 
	here BL(branch with link) command corrupts the LR(link register) value as it stores the address of this fcn 
	to LR to get back from get_sp_value so we need to save it first
	 */
	__asm volatile("PUSH {LR}");
	// get value of PSP of the current task
	__asm volatile("BL get_sp_value"); // calls get_sp_value and returns with R0 containing the value
	// initialize PSP
	__asm volatile("MSR PSP,R0");
	// POP the original value of LR from stack back to LR
	__asm volatile("POP {LR}");

	// change SP to PSP
	__asm volatile("MOV R0,#0x02"); //store the value to config into R0 first, R0 is spare now bc the PSP value is already loaded to PSP
	__asm volatile("MSR CONTROL,R0 ");
	__asm volatile("BX LR"); // return
}

// dispatches the current task; called once at startup to enter the scheduler from main
void os_task_start(void)
{
	user_tasks[current_task].task_handler();
}

// blocks the current task for tick_count ticks and yields to the next ready task
void os_task_delay(uint32_t tick_count)
{
	// disable interrupt
	INTERRUPT_DISABLE();
	
	// only block the task if it not the idle task
	if(current_task != 0)
	{
		// set wakeup time for the task
		user_tasks[current_task].wakeup_tick = systick_count + tick_count;
		// change to blocked state
		user_tasks[current_task].current_state = TASK_BLOCKED_STATE;
		// pend pendSV exception
		pend_pendsv(); // switches to another task to allow other tasks to run
	}

	// enable interrupt
	INTERRUPT_ENABLE();
}

// increments the tick counter on every SysTick interrupt
static void update_tick_count(void)
{
	systick_count++;
}

// triggers a PendSV exception to perform a context switch at the lowest exception priority
static void pend_pendsv(void)
{
	uint32_t *p_ICSR = (uint32_t*) 0xE000ED04;
	// pend the pendSV exception
	*p_ICSR |= (1 << 28);
}

// transitions blocked tasks to ready if their wakeup tick has been reached
static void unblock_tasks(void)
{
	// unblock any tasks that are qualified for running
	for (int i = 1; i < MAX_TASKS; i++)//ignores the idle task
	{
		if (user_tasks[i].current_state != TASK_READY_STATE)
		{
			// blocking period has elapsed
			if (systick_count >= user_tasks[i].wakeup_tick)
			{
				user_tasks[i].current_state = TASK_READY_STATE;
			}
		}
	}
}

// selects the next ready task in round-robin order, falls back to idle if all tasks are blocked
static void update_next_task(void)
{
	// finds the next task that is ready to run
	int state = TASK_BLOCKED_STATE;
	for (int i = 0; i < MAX_TASKS; i++)
	{
		current_task++;
		current_task %= MAX_TASKS; // current task will always be 0-3 inclusive and gets back to 0 when it is 4 -> round-robin
		state = user_tasks[current_task].current_state;

		// only break if the task is not the idle_task, bc idle_task's state is always TASK_READY_STATE
		if ( (state == TASK_READY_STATE) && (current_task != 0) )
		{
			break;
		}
	}

	if (state == TASK_BLOCKED_STATE)
	{
		// no tasks are free, go idle
		current_task = 0;
	}
}

// enables UsageFault, BusFault, and MemManageFault so they trap as their own exceptions
void os_enable_processor_faults(void)
{
	uint32_t *p_SHCSR = (uint32_t*)0xE000ED24;
	*p_SHCSR |= (1 << 18); // usage fault
	*p_SHCSR |= (1 << 17); // bus fault
	*p_SHCSR |= (1 << 16); // mem fault
}

// context switch handler: saves SF2 of current task, selects next task, restores its SF2 (SF1 is saved/restored automatically by hardware)
__attribute__((naked)) void PendSV_Handler(void)
{
	/*
	naked here to save SF2 from task A's PSP and restores SF2 from task B's PSP — two different stacks.
	A compiler prologue/epilogue assumes push and pop are symmetric on the same stack, so it would
	corrupt both tasks' stacks. The return must also be a bare BX LR with EXC_RETURN in LR to
	trigger exception return; a generated epilogue would emit its own return sequence instead.
	*/

	// get current running task's PSP value
	__asm volatile("MRS R0, PSP"); // store the PSP value to R0

	// using the PSP value, store SF2(R4 to R11)
	/*
	notes:  can't just use PUSH b/c the handler always uses MSP, which will only push the values to the MSP stack, not the task private stack
			so use STMDB to save the values at the PSP address extracted into R0
			STMDB stores thos values into multiple registers and decrement first then store
	*/
	__asm volatile("STMDB R0!, {R4-R11}");  // read registers R4-R11, store into memory at R0, R0 is updated after each acess
											// "!" the final address that is stored will be loaded back to R0

	// save the current value of PSP
	__asm volatile("PUSH {LR}"); // push LR onto the stack first, because c fcn calls will corrupt LR
	__asm volatile("BL save_sp_value"); // R0 is passed as parameter by default
	
	// decide next task to run
	__asm volatile("BL update_next_task");

	// get the task's past PSP value, return value is stored in R0
	__asm volatile("BL get_sp_value");

	// using that PSP value retrieve SF2(R4 to R11), SF1 will be automatically retrieved when exiting handler
	 __asm volatile("LDMIA R0!, {R4-R11}"); // LDMIA: load multiple from memory to register, R0 is starting memory address to read from,
											// "!" to increment address after each access, R4-R11 are registers to load into

	// update PSP and exit
	 __asm volatile("MSR PSP, R0");

	// pop back the LR pushed to stack
	 __asm volatile("POP {LR}");

	// return
	 __asm volatile("BX LR");
}

void SysTick_Handler(void)
{
	update_tick_count();

	unblock_tasks();

	pend_pendsv();
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

