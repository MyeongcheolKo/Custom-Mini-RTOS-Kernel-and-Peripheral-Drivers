/*
 * main.h
 *
 *  Created on: Dec 22, 2025
 *      Author: krisko
 */

#ifndef MAIN_H_
#define MAIN_H_

#define OS_SCHEDULER_STACK_WORDS 256

//systick timer macros
#define TICK_HZ 1000U
#define HSI_CLOCK 16000000U
#define SYSTICK_CLOCK HSI_CLOCK

#define INTERRUPT_DISABLE() do{__asm volatile("MOV R0,#0X1"); __asm volatile("MSR PRIMASK,R0");} while(0)
#define INTERRUPT_ENABLE() do{__asm volatile("MOV R0,#0X0"); __asm volatile("MSR PRIMASK,R0");} while(0)

#endif /* MAIN_H_ */
