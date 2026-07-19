/*
 * scheduler.h
 *
 *  Created on: Dec 23, 2025
 *      Author: krisko
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "../config/osConfig.h"

typedef enum {
	OS_OK = 0,
	OS_ERR_INVALID_PRIORITY,
	OS_ERR_MAX_TASKS,
	OS_ERR_NULL_PTR
} os_err_t;

typedef enum {
	TASK_READY,
	TASK_RUNNING,
	TASK_BLOCKED
} task_state_t;

typedef enum {
	BLOCKED_NONE = 0, // task is not blocked
	BLOCKED_DELAY,
	BLOCKED_SEM,
	BLOCKED_MUTEX
} task_block_reason_t;

typedef struct {
	uint32_t stack_pointer;
	uint32_t wakeup_tick;
	task_state_t current_state;
	task_block_reason_t block_reason;
	uint8_t priority_level; // lower value = higher priority, 0 is reserved for idle task
	void (*task_handler)(void);
} TCB_t;

/*---------- public APIs ----------*/

// sets scheduler up and starts the kernel, does not return
void os_kernel_start(void);

// registers a task with its own private stack; lower priority value = higher priority, valid range 1..OS_PRIORITY_LOWEST (0 is reserved for the idle task)
// returns OS_OK, or OS_ERR_MAX_TASKS / OS_ERR_INVALID_PRIORITY / OS_ERR_NULL_PTR on failure
os_err_t os_task_create(void (*task_handler)(void), uint8_t priority, uint32_t *task_stack_base, uint32_t task_stack_size);

// blocks the calling task for tick_count SysTick ticks and yields to the next ready task (no-op when called from the idle task)
void os_task_delay(uint32_t tick_count);

// optional user hook for the idle task, called once per idle loop iteration
__attribute__((weak)) void os_idle_task_hook(void) { /*default is empty, user can override this function*/ }


#endif /* SCHEDULER_H_ */
