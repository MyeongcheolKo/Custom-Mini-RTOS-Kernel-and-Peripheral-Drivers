#ifndef KERNEL_INTERNAL_H_
#define KERNEL_INTERNAL_H_

#include <stdint.h>

/*----- internal kernel interface (core + port use — not application API) -----*/

// called by SysTick_Handler to update tick count, unblock tasks, and pend PendSV
void os_tick(void); 

// returns the saved PSP of the current task from its TCB
uint32_t os_get_sp_value(void);

// saves the current task's PSP into its TCB
void os_save_sp_value(uint32_t current_psp_val);

// schedules the next ready task in priority round-robin order, falls back to idle if all tasks are blocked
void os_schedule_next_task(void);

#endif /* KERNEL_INTERNAL_H_ */
