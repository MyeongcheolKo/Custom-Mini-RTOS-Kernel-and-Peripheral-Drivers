/*
 * main.h
 *
 *  Created on: Dec 22, 2025
 *      Author: krisko
 */

#ifndef MAIN_H_
#define MAIN_H_

#define MAX_TASKS 5

/* stack memory calculations */
#define TASK_STACK_SIZE 1024U // 1 KB FOR EACH TASK PRIVATE STACK
#define SCHEDU_STACK_SIZE 1024U // 1 KB FOR THE SCHEDULER STACK AS WELL

#define SRAM_START 0x20000000U
#define SRAM_SIZE (128U * 1024U)
#define SRAM_END (SRAM_START + SRAM_SIZE)

// stack memory locations
#define T1_STACK_START (SRAM_END)
#define T2_STACK_START (SRAM_END - TASK_STACK_SIZE)
#define T3_STACK_START (SRAM_END - (2 * TASK_STACK_SIZE))
#define T4_STACK_START (SRAM_END - (3 * TASK_STACK_SIZE))
#define IDLE_STACK_START (SRAM_END - (4 * TASK_STACK_SIZE))
#define SCHEDULER_STACK_START (SRAM_END - (5 * TASK_STACK_SIZE))

//systick timer macros
#define TICK_HZ 1000U
#define HSI_CLOCK 16000000U
#define SYSTICK_CLOCK HSI_CLOCK

//dummy stack macros
#define DUMMY_XPSR 0x01000000U // only t-bit is needed to set (use Thumb instructions)

#define TASK_READY_STATE 0x00
#define TASK_BLOCKED_STATE 0xFF

#define INTERRUPT_DISABLE() do{__asm volatile("MOV R0,#0X1"); __asm volatile("MSR PRIMASK,R0");} while(0)
#define INTERRUPT_ENABLE() do{__asm volatile("MOV R0,#0X0"); __asm volatile("MSR PRIMASK,R0");} while(0)

#endif /* MAIN_H_ */
