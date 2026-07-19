/*
 * scheduler.c
 *
 *  Created on: Dec 23, 2025
 *      Author: krisko
 */
#include <stdint.h>
#include <stdio.h>
#include "scheduler.h"

// dummy stack macros
#define DUMMY_STACK_XPSR 0x01000000U // only t-bit is needed to set (use Thumb instructions)
#define EXC_RETURN_THREAD_PSP 0xFFFFFFFD

#define INTERRUPT_DISABLE() do{__asm volatile("MOV R0,#0X1"); __asm volatile("MSR PRIMASK,R0");} while(0)
#define INTERRUPT_ENABLE() do{__asm volatile("MOV R0,#0X0"); __asm volatile("MSR PRIMASK,R0");} while(0)

static uint32_t scheduler_stack[OS_SCHEDULER_STACK_WORDS] __attribute__((aligned(8)));
static uint32_t task_count = 1; // idle task always exists
static uint32_t current_task = 0;		// start with idle task
static TCB_t user_tasks[OS_MAX_TASKS];
static uint32_t systick_count = 0;
static uint32_t idle_task_stack[OS_IDLE_STACK_WORDS] __attribute__((aligned(8)));

/* os private function prototypes */
static void init_systick_timer(uint32_t tick_hz);
static __attribute__ ((naked)) void init_scheduler_stack(uint32_t schedu_top_of_stack);
static void init_idle_task(void);
static void set_pendSV_priority_lowest(void);
static void switch_to_psp(void);
static uint32_t init_task_stack_frame(void (*task_handler)(void), uint32_t *task_stack_base, uint32_t task_stack_size);
static __attribute__((used)) void save_sp_value(uint32_t current_psp_val);
static __attribute__((used)) uint32_t get_sp_value(void);
static __attribute__((used)) void update_tick_count(void);
static void pend_pendsv(void);
static void unblock_tasks(void);
static __attribute__((used)) void schedule_next_task(void);
static void idle_task_hanlder(void);

/*
starts the kernel: sets up the scheduler stack, idle task, PendSV priority, and SysTick tick, switches to PSP, then dispatches the first task
call once from main after all os_task_create calls; never returns
*/
void os_kernel_start(void)
{
	uint32_t scheduler_stack_top = (uint32_t)(scheduler_stack + sizeof(scheduler_stack) / sizeof(scheduler_stack[0]));
	init_scheduler_stack(scheduler_stack_top);
	
	init_idle_task();
	
	set_pendSV_priority_lowest();
	
	init_systick_timer(OS_TICK_HZ);

	switch_to_psp();

	schedule_next_task();
	user_tasks[current_task].task_handler();
}

os_err_t os_task_create(void (*task_handler)(void), uint8_t priority, uint32_t *task_stack_base, uint32_t task_stack_size)
{
	if (task_count >= OS_MAX_TASKS)
		return OS_ERR_MAX_TASKS;
	if (priority <= OS_PRIORITY_HIGHEST)
		return OS_ERR_INVALID_PRIORITY; // priority 0 is reserved for idle task
	if (task_stack_base == NULL)
		return OS_ERR_NULL_PTR;

		
	TCB_t *tcb = &user_tasks[task_count];
	tcb->task_handler = task_handler;
	tcb->priority_level = priority;
	tcb->current_state = TASK_READY;
	tcb->stack_pointer = init_task_stack_frame(task_handler, task_stack_base, task_stack_size);
	// no need to set block_reason bc it is zero initialized and BLOCKED_NONE is 0
	
	task_count++;
	return OS_OK;
}

// blocks the current task for tick_count ticks and yields to the next ready task
void os_task_delay(uint32_t tick_count)
{
	// disable interrupt
	INTERRUPT_DISABLE();

	// only block the task if it not the idle task
	if (current_task != 0)
	{
		// set wakeup time for the task
		user_tasks[current_task].wakeup_tick = systick_count + tick_count;
		// change to blocked state and specify reason
		user_tasks[current_task].block_reason = BLOCKED_DELAY;
		user_tasks[current_task].current_state = TASK_BLOCKED;
		// pend pendSV exception
		pend_pendsv(); // switches to another task to allow other tasks to run
	}

	// enable interrupt
	INTERRUPT_ENABLE();
}

/*------------static functions-------------*/

// configures SysTick to fire at tick_hz interrupts per second using the processor clock
static void init_systick_timer(uint32_t tick_hz)
{
	uint32_t count_val = (OS_SYSTICK_CLOCK_HZ / tick_hz) - 1;
	uint32_t *p_SYST_RVR = (uint32_t *)0xE000E014; // systick reload value register

	// clear the value
	*p_SYST_RVR &= ~(0x00FFFFFF);

	// load the value that will be reloaded into the register when it reaches 0
	// achieve the desired systick freq by configuring this
	*p_SYST_RVR |= count_val;

	uint32_t *p_SYST_CSR = (uint32_t *)0xE000E010; //  SysTick Control and Status Register
	*p_SYST_CSR |= (1 << 1); // enable the systick exception request
	*p_SYST_CSR |= (1 << 2); // use processor clock
	*p_SYST_CSR |= 1; // enable the counter
}

// sets MSP to the top of the scheduler stack where all the handlers run
static __attribute__((naked)) void init_scheduler_stack(uint32_t scheduler_top_of_stack)
{
	/*
	naked func here bc MSR MSP writes to the stack before a valid stack exists, so a compiler generated
	prologue that pushes to MSP would corrupt memory before the stack is set up

	naked forces compiler to not generate prologue and epilogues
	*/

	// change msp (handler sp) to appropriate address of the scheduler stack
	__asm volatile("MSR MSP,%0" : : "r"(scheduler_top_of_stack)); // MSR(move to special register)
	// return to the caller func,
	__asm volatile("BX LR"); // BX (branch and exchange to a adress) LR(address of the caller)
}

// builds a dummy exception stack frame for idle task
static void init_idle_task(void)
{
	TCB_t *tcb = &user_tasks[0];
	tcb->task_handler = idle_task_hanlder;
	tcb->priority_level = 0; // priority 0 reserved for idle task, highest priority
	tcb->current_state = TASK_READY;
	tcb->stack_pointer = init_task_stack_frame(idle_task_hanlder, idle_task_stack, sizeof(idle_task_stack));
}

// configure PendSV to lowest priority so it is only triggered when no other exceptions are active
static void set_pendSV_priority_lowest(void)
{
	uint32_t *p_SHPR3 = (uint32_t *)0xE000ED20; // system Handler Priority Register 3
	*p_SHPR3 |= (0xFFU << 16); // PRI_14 (PendSV) = bits [23:16]
}

// switches the active stack pointer from MSP to PSP so tasks run on their own private stacks
static __attribute__((naked)) void switch_to_psp(void)
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
	__asm volatile("MOV R0,#0x02"); // store the value to config into R0 first, R0 is spare now bc the PSP value is already loaded to PSP
	__asm volatile("MSR CONTROL,R0 ");
	__asm volatile("BX LR"); // return
}

// builds the initial dummy stack frame at the top of a stack, returns the PSP
static uint32_t init_task_stack_frame(void (*task_handler)(void), uint32_t *task_stack_base, uint32_t task_stack_size)
{
	// stack intialization (full descending)
	uint32_t *sp = task_stack_base + (task_stack_size / sizeof(uint32_t));  // point to the top of the stack since full descending
	/*  initialize a dummy stack frame for the task
		when first time the tasks are run, there were no past context
		so no context to retrieve as there are nothing on the stack
		make dummy stack frames:
		general registers -> all set to 0
		xPSR -> only need t bit to be 1
		PC -> the corresponding task handler
		LR -> EXC_RETURN, should be 0xFFFFFFFD as we need return to thread with PSP
	*/
	// Configure xPSR(program status register)
	*(--sp) = DUMMY_STACK_XPSR; 
	// PC(program counter), hold address of the next instruction to execute, here direct to the corresponding task_handler
	*(--sp) = (uint32_t)task_handler;
	/*
	LR(link register), should normally hold address to the next instruction from where this func is called
	but here use exception return with thread mode and psp stack pointer. when entering an exception
	LR will be set to an EXC_RETURN value and LR will be loaded into PC to signal the exception is complete
	Context switching happens in PendSV, which is an exception.
	Hardware will set LR to an EXC_RETURN at entry of an exception but keeping the value consistent here makes sense 
	as we need smth to sarisfy the stack frame layout
	*/
	*(--sp) = EXC_RETURN_THREAD_PSP;
	// zero out all general registers
	for (int j = 0; j < 13; j++) *(sp--) = 0;

	return (uint32_t)sp;
}

// saves the current task's PSP into its TCB
static void save_sp_value(uint32_t current_psp_val)
{
	user_tasks[current_task].stack_pointer = current_psp_val;
}

// returns the saved PSP of the current task from its TCB
static uint32_t get_sp_value(void)
{
	return user_tasks[current_task].stack_pointer;
}

// increments the tick counter on every SysTick interrupt
static void update_tick_count(void)
{
	systick_count++;
}

// triggers a PendSV exception to perform a context switch at the lowest exception priority
static void pend_pendsv(void)
{
	uint32_t *p_ICSR = (uint32_t *)0xE000ED04;
	// pend the pendSV exception
	*p_ICSR |= (1 << 28);
}

// transitions TASK_BLOCKED tasks to TASK_READY if their wakeup tick has been reached
static void unblock_tasks(void)
{
	if (task_count <= 1) return;
	// unblock any tasks that are qualified for running
	for (int i = 1; i < task_count; i++) // ignores the idle task
	{
		// wake up any task that has a wakeup deadline(task_delay wake tick or semaphore and mutex timeout) 
		// if it is blocked and the wakeup tick has been reached, regardless of the block reason
		if (user_tasks[i].current_state == TASK_BLOCKED && 
			user_tasks[i].wakeup_tick != 0 && // no wakeup tick, wait forever, don't unblock
			(int32_t)(systick_count - user_tasks[i].wakeup_tick) >= 0) // Compare the difference so that it works when systick_count wraps around
		{
			user_tasks[i].current_state = TASK_READY;
			user_tasks[i].block_reason = BLOCKED_NONE;
			user_tasks[i].wakeup_tick = 0; // reset wakeup tick
		}
	}
}

// schedules the next ready task in priority round-robin order, falls back to idle if all tasks are blocked
static void schedule_next_task(void)
{
	// task that was running (and didn't block) goes back to TASK_READY
    if (user_tasks[current_task].current_state == TASK_RUNNING) 
		user_tasks[current_task].current_state = TASK_READY;
	
	uint8_t task_to_run = 0;
	uint8_t highest_priority = 255;
	// finds the next task that is ready to run with highest priority (lowest priority number)
	if (task_count <= 1) return;
	for (int i = 1; i < task_count; i++)
	{
		if (user_tasks[i].current_state == TASK_READY && 
			user_tasks[i].priority_level < highest_priority)
		{
			highest_priority = user_tasks[i].priority_level;
			task_to_run = i;
		}
	}

	// task_to_run = 0 when no tasks are free, go idle
	current_task = task_to_run;
	user_tasks[current_task].current_state = TASK_RUNNING;
}

// the internal used idle task handler called by the scheduler, users should override os_idle_task_hook() instead of this
static void idle_task_hanlder(void)
{
	while(1)
	{
		os_idle_task_hook(); // user optional work
		__asm volatile("WFI"); // sleep core until next interrupt (SysTick wakes it)
	}
}

/*---------Handlers---------*/

void SysTick_Handler(void)
{
	update_tick_count();

	unblock_tasks();

	pend_pendsv();
}

// performs the context switch: saves SF2 of current task, selects next task, restores its SF2 (SF1 is saved/restored automatically by hardware)
__attribute__((naked)) void PendSV_Handler(void)
{
	/*
	naked here to save SF2 from task A's PSP and restores SF2 from task B's PSP — two different stacks.
	A compiler prologue/epilogue assumes push and pop are symmetric on the same stack, so it would
	corrupt both tasks' stacks. The return must also be a bare BX LR with EXC_RETURN in LR to
	trigger exception return, a generated epilogue would emit its own return sequence instead.
	*/

	// store the current running task's PSP value to R0
	__asm volatile("MRS R0, PSP");

	/*
	using the PSP value, store SF2(R4 to R11)
	notes:  can't just use PUSH b/c the handler always uses MSP, which will only push the values to the MSP stack, not the task private stack
			so save the values at the PSP address extracted into R0 by using STMDB 
			STMDB stores those values into multiple registers and decrement first then store
	*/
	__asm volatile("STMDB R0!, {R4-R11}"); // read registers R4-R11, store into memory at R0, R0 is updated after each acess
										   // "!" the final address that is stored will be loaded back to R0

	// save the current value of PSP
	__asm volatile("PUSH {LR}");		// push LR (holds EXC_RETURN) onto the stack first, because BL calls will corrupt LR
	__asm volatile("BL save_sp_value"); // R0 is passed as parameter by default

	// decide next task to run
	__asm volatile("BL schedule_next_task");

	// get the task's past PSP value, return value is stored in R0
	__asm volatile("BL get_sp_value");

	// using that PSP value retrieve SF2(R4 to R11), SF1 will be automatically retrieved when exiting handler
	__asm volatile("LDMIA R0!, {R4-R11}"); // LDMIA: load multiple from memory to register, R0 is starting memory address to read from,
										   // "!" to increment address after each access, R4-R11 are registers to load into

	// update PSP and exit
	__asm volatile("MSR PSP, R0");

	// restore EXC_RETURN into LR
	__asm volatile("POP {LR}");

	// performs the exception return, and hardware pops SF1 (including PC) from the newly selected task's PSP
	__asm volatile("BX LR");
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
