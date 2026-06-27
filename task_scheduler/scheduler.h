/*
 * scheduler.h
 *
 *  Created on: Dec 23, 2025
 *      Author: krisko
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#define EXC_RETURN_THREAD_PSP		0xFFFFFFFD

typedef struct
{
	uint32_t stack_pointer;
	uint32_t wakeup_tick;
	uint8_t current_state;
	void (*task_handler)(void);
}TCB_t;

/* public interfaces */
void os_init_systick_timer(uint32_t tick_hz);
__attribute__ ((naked)) void os_init_scheduler_stack(uint32_t schedu_top_of_stack);
void os_init_task_stack(void);
void os_switch_to_psp(void);
void os_enable_processor_faults(void);
void os_task_start(void);
void os_task_delay(uint32_t tick_count);


#endif /* SCHEDULER_H_ */
