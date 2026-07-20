/*
 * kernel.c
 *
 *  Created on: Dec 23, 2025
 *      Author: krisko
 */
#include <stdint.h>
#include <stdio.h>
#include "os.h"
#include "kernel_internal.h"
#include "../port/arm/cortex_m4/port.h"

static uint32_t scheduler_stack[OS_SCHEDULER_STACK_WORDS] __attribute__((aligned(8)));
static uint32_t task_count = 1; // idle task always exists
static uint32_t current_task = 0;		// start with idle task
static TCB_t user_tasks[OS_MAX_TASKS];
static uint32_t systick_count = 0;
static uint32_t idle_task_stack[OS_IDLE_STACK_WORDS] __attribute__((aligned(8)));

/* os private function prototypes */
static void init_idle_task(void);
static __attribute__((used)) void update_tick_count(void);
static void unblock_tasks(void);
static void idle_task_handler(void);

/*-------------- public APIs ---------------*/

/*
starts the kernel: sets up the scheduler stack, idle task, PendSV priority, and SysTick tick, switches to PSP, then dispatches the first task
call once from main after all os_task_create calls; never returns
*/
void os_kernel_start(void)
{
	uint32_t scheduler_stack_top = (uint32_t)(scheduler_stack + sizeof(scheduler_stack) / sizeof(scheduler_stack[0]));
	port_init_scheduler_stack(scheduler_stack_top);
	init_idle_task();
	port_set_pendSV_priority_lowest();
	port_init_systick(OS_TICK_HZ);
	os_schedule_next_task();
	port_switch_to_psp();
	user_tasks[current_task].task_handler(); // never returns
}

/*
registers a task with its own private stack; lower priority value = higher priority, valid range 1..OS_PRIORITY_LOWEST (0 is reserved for the idle task)
returns OS_OK, or OS_ERR_MAX_TASKS / OS_ERR_INVALID_PRIORITY / OS_ERR_NULL_PTR on failure
*/
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
	tcb->stack_pointer = port_init_task_stack_frame(task_handler, task_stack_base, task_stack_size);
	// no need to set block_reason bc it is zero initialized and BLOCKED_NONE is 0
	
	task_count++;
	return OS_OK;
}

// blocks the current task for tick_count ticks and yields to the next ready task
void os_task_delay(uint32_t tick_count)
{
	// disable interrupt
	PORT_INTERRUPT_DISABLE();

	// only block the task if it not the idle task
	if (current_task != 0)
	{
		// set wakeup time for the task
		user_tasks[current_task].wakeup_tick = systick_count + tick_count;
		// change to blocked state and specify reason
		user_tasks[current_task].block_reason = BLOCKED_DELAY;
		user_tasks[current_task].current_state = TASK_BLOCKED;
		// pend pendSV exception
		port_yield(); // switches to another task to allow other tasks to run
	}

	// enable interrupt
	PORT_INTERRUPT_ENABLE();
}

/*----- internal kernel interface (core + port use — not application API) -----*/

// called by SysTick_Handler to update tick count, unblock tasks, and pend PendSV
void os_tick(void)
{
	update_tick_count();

	unblock_tasks();

	port_yield();
}

// returns the saved PSP of the current task from its TCB
uint32_t os_get_sp_value(void)
{
	return user_tasks[current_task].stack_pointer;
}

// saves the current task's PSP into its TCB
void os_save_sp_value(uint32_t current_psp_val)
{
	user_tasks[current_task].stack_pointer = current_psp_val;
}

// schedules the next ready task in priority round-robin order, falls back to idle if all tasks are blocked
void os_schedule_next_task(void)
{
	// task that was running (and didn't block) goes back to TASK_READY
    if (user_tasks[current_task].current_state == TASK_RUNNING) 
		user_tasks[current_task].current_state = TASK_READY;
	
	uint8_t task_to_run = 0;
	uint8_t highest_priority = 255;
	if (task_count <= 1) return;
	// finds the next task that is ready to run in round robin order with highest priority (lowest priority number)
	for (int i = current_task + 1; i < task_count; i++) // start after the current task
	{
		if (user_tasks[i].current_state == TASK_READY && user_tasks[i].priority_level < highest_priority)
		{
			highest_priority = user_tasks[i].priority_level;
			task_to_run = i;
		}
	}
	for (int i = 1; i <= current_task; i++) // reaches the current task last so it only runs if no other tasks with higher priority are ready, therefor round robin
	{
		if (user_tasks[i].current_state == TASK_READY && user_tasks[i].priority_level < highest_priority)
		{
			highest_priority = user_tasks[i].priority_level;
			task_to_run = i;
		}
	}

	// task_to_run = 0 when no tasks are free, go idle
	current_task = task_to_run;
	user_tasks[current_task].current_state = TASK_RUNNING;
}

/*------------ core private functions -------------*/

// builds a dummy exception stack frame for idle task
static void init_idle_task(void)
{
	TCB_t *tcb = &user_tasks[0];
	tcb->task_handler = idle_task_handler;
	tcb->priority_level = 0; // priority 0 reserved for idle task, highest priority
	tcb->current_state = TASK_READY;
	tcb->stack_pointer = port_init_task_stack_frame(idle_task_handler, idle_task_stack, sizeof(idle_task_stack));
}

// increments the tick counter on every SysTick interrupt
static void update_tick_count(void)
{
	systick_count++;
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

// the internal used idle task handler called by the scheduler, users should override os_idle_task_hook() instead of this
static void idle_task_handler(void)
{
	while(1)
	{
		os_idle_task_hook(); // user optional work
		PORT_WAIT_FOR_INTERRUPT(); // sleep core until next interrupt (SysTick wakes it)
	}
}
