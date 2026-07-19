/*
 * rcc.c
 *
 *  Created on: Jan 5, 2026
 *      Author: krisko
 */

#include <stdint.h>
#include "rcc.h"
/*
 * @func:			RCC_get_pclk1
 *
 * @brief:			This function calculates the clock speed of APB1
 *
 * @return:			APB1 clock speed
 *
 * @note:			only HSI and HSE are considered for system clock source, PLL is not considered
 */
uint32_t RCC_get_pclk1(void)
{
	static const uint32_t possible_pscals[] = {2,4,8,16,64,128,256,512};
	uint32_t system_clk, AHB_pscal, APB1_pscal;

	//find system clock source speed
	uint32_t clk_src = (RCC->CFGR >> 2) & 0b11;
	if(clk_src == 0){	//HSI
		system_clk = 16000000;
	}
	else if (clk_src == 1)	//HSE
	{
		system_clk = 8000000;
	}
	else
	{
		// PLL not supported, assume HSI as fallback
		system_clk = 16000000;
	}
	//find AHB prescalar
	uint32_t HPRE = (RCC->CFGR >> 4) & 0b1111;
	if(HPRE < 8) 	//vals below 0b1000 are all not divided by
		AHB_pscal = 1;
	else			//vals at 0b1000 and above are divided by
		AHB_pscal = possible_pscals[HPRE - 8];

	//find APB1 prescalar, I2C peripherals all under APB1
	uint32_t PPRE1 = (RCC->CFGR >> 10) & 0b111;
	if(PPRE1 < 4)
		APB1_pscal = 1;
	else
		APB1_pscal = possible_pscals[PPRE1 - 4];

	return (system_clk / AHB_pscal) / APB1_pscal;
}

/*
 * @func:			RCC_get_pclk1
 *
 * @brief:			This function calculates the clock speed of APB2
 *
 * @return:			APB2 clock speed
 *
 * @note:			only HSI and HSE are considered for system clock source, PLL is not considered
 */
uint32_t RCC_get_pclk2(void)
{
	static const uint32_t possible_pscals[] = {2,4,8,16,64,128,256,512};
	uint32_t system_clk, AHB_pscal, APB2_pscal;

	//find system clock source speed
	uint32_t clk_src = (RCC->CFGR >> 2) & 0b11;
	if(clk_src == 0){	//HSI
		system_clk = 16000000;
	}
	else if (clk_src == 1)	//HSE
	{
		system_clk = 8000000;
	}
	else
	{
		// PLL not supported, assume HSI as fallback
		system_clk = 16000000;
	}
	//find AHB prescalar
	uint32_t HPRE = (RCC->CFGR >> 4) & 0b1111;
	if(HPRE < 8) 	//vals below 0b1000 are all not divided by
		AHB_pscal = 1;
	else			//vals at 0b1000 and above are divided by
		AHB_pscal = possible_pscals[HPRE - 8];

	//find APB2 prescalar, I2C peripherals all under APB2
	uint32_t PPRE2 = (RCC->CFGR >> 13) & 0b111;
	if(PPRE2 < 4)
		APB2_pscal = 1;
	else
		APB2_pscal = possible_pscals[PPRE2 - 4];

	return (system_clk / AHB_pscal) / APB2_pscal;
}
