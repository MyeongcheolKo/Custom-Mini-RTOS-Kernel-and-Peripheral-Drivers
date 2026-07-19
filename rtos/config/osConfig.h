#ifndef OS_CONFIG_H_
#define OS_CONFIG_H_

/* OS configuration parameters */
#define OS_TICK_HZ 1000U // number of ticks per second for systick timer
#define OS_SYSTICK_CLOCK_HZ 16000000U // hardware systick timer clock frequency, HSI clock in this case

#define OS_SCHEDULER_STACK_WORDS 256U
#define OS_IDLE_STACK_WORDS 64U

#define OS_MAX_TASKS 20U

#define OS_PRIORITY_HIGHEST 0
#define OS_PRIORITY_LOWEST 31U

#endif
