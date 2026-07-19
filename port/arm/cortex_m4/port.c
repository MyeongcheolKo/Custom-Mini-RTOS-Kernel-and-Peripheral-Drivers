#include "port.h"
#include "scheduler_internal.h"
#include "osConfig.h"

// dummy stack macros
#define DUMMY_STACK_XPSR 0x01000000U // only t-bit is needed to set (use Thumb instructions)
#define EXC_RETURN_THREAD_PSP 0xFFFFFFFD

// sets MSP to the top of the scheduler stack where all the handlers run
__attribute__((naked)) void port_init_scheduler_stack(uint32_t scheduler_top_of_stack)
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

// configure PendSV to lowest priority so it is only triggered when no other exceptions are active
void port_set_pendSV_priority_lowest(void)
{
	uint32_t *p_SHPR3 = (uint32_t *)0xE000ED20; // system Handler Priority Register 3
	*p_SHPR3 |= (0xFFU << 16); // PRI_14 (PendSV) = bits [23:16]
}

// configures SysTick to fire at tick_hz interrupts per second using the processor clock
void port_init_systick(uint32_t tick_hz)
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

// switches the active stack pointer from MSP to PSP so tasks run on their own private stacks
__attribute__((naked)) void port_switch_to_psp(void)
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
	__asm volatile("BL os_get_sp_value"); // calls os_get_sp_value and returns with R0 containing the value
	// initialize PSP
	__asm volatile("MSR PSP,R0");
	// POP the original value of LR from stack back to LR
	__asm volatile("POP {LR}");

	// change SP to PSP
	__asm volatile("MOV R0,#0x02"); // store the value to config into R0 first, R0 is spare now bc the PSP value is already loaded to PSP
	__asm volatile("MSR CONTROL,R0 ");
	__asm volatile("BX LR"); // return
}

// builds the initial dummy stack frame at the top of a stack for the task, returns the PSP
uint32_t port_init_task_stack_frame(void (*task_handler)(void), uint32_t *task_stack_base, uint32_t task_stack_size)
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

// triggers a PendSV exception to perform a context switch at the lowest exception priority
void port_yield(void)
{
	uint32_t *p_ICSR = (uint32_t *)0xE000ED04;
	// pend the pendSV exception
	*p_ICSR |= (1 << 28);
}

/*--------------Handlers--------------*/

void SysTick_Handler(void)
{
    os_tick();
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
	__asm volatile("BL os_save_sp_value"); // R0 is passed as parameter by default

	// decide next task to run
	__asm volatile("BL os_schedule_next_task");

	// get the task's past PSP value, return value is stored in R0
	__asm volatile("BL os_get_sp_value");

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
