/*
 * scheduler.h
 *
 *  Created on: Dec 23, 2025
 *      Author: krisko
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "main.h"
#include "osConfig.h"

#define OS_IDLE_TASK_STACK_SIZE 256U
#define OS_IDLE_STACK_WORDS (OS_IDLE_TASK_STACK_SIZE / sizeof(uint32_t))

// dummy stack macros
#define DUMMY_XPSR 0x01000000U // only t-bit is needed to set (use Thumb instructions)

#define OS_MAX_TASKS 20
#define EXC_RETURN_THREAD_PSP 0xFFFFFFFD

#define OS_PRIORITY_HIGHEST 0
#define OS_PRIORITY_LOWEST 31


typedef enum {
	OS_OK = 0,
	OS_ERR_INVALID_PRIORITY,
	OS_ERR_MAX_TASKS,
	OS_ERR_NULL_PTR
} os_err_t;

/* TASK_STATES */
typedef enum {
	TASK_READY,
	TASK_RUNNING,
	TASK_BLOCKED
} task_state_t;

typedef struct {
	uint32_t stack_pointer;
	uint32_t wakeup_tick;
	task_state_t current_state;
	uint8_t priority_level; // lower value = higher priority, 0 is reserved for idle task
	void (*task_handler)(void);
} TCB_t;

/* public interfaces */
void os_init_systick_timer(uint32_t tick_hz);
__attribute__ ((naked)) void os_init_scheduler_stack(uint32_t schedu_top_of_stack);
os_err_t os_task_create(void (*task_handler)(void), uint8_t priority, uint32_t *task_stack_base, uint32_t task_stack_size);
void os_init(void);
void os_switch_to_psp(void);
void os_enable_processor_faults(void);
void os_task_start(void);
void os_task_delay(uint32_t tick_count);

__attribute__((weak)) void os_idle_task_hook(void) { /*default is nothing, user can override this function*/ }


#endif /* SCHEDULER_H_ */
