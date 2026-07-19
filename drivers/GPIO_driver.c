/*
 * STM32F446xx_GPIO_driver.c
 *
 *  Created on: Dec 26, 2025
 *      Author: krisko
 */
#include "GPIO_driver.h"


/*
 * @func:			GPIO_clock_control
 *
 * @brief:			This function enables or disables the peripheral clock for the given GPIO port
 *
 * @param[in]:		base address of GPIO peripheral
 * @param[in]:		ENABLE or DISABLE macros
 *
 * @return: 		none
 */
void GPIO_clock_control(GPIO_reg_t *p_GPIOx, uint8_t enable)
{
	if(enable == ENABLE)
	{
		if(p_GPIOx == GPIOA)
		{
			GPIOA_PCLK_EN();
		}
		else if(p_GPIOx == GPIOB)
		{
			GPIOB_PCLK_EN();
		}
		else if(p_GPIOx == GPIOC)
		{
			GPIOC_PCLK_EN();
		}
		else if(p_GPIOx == GPIOD)
		{
			GPIOD_PCLK_EN();
		}
		else if(p_GPIOx == GPIOE)
		{
			GPIOE_PCLK_EN();
		}
		else if(p_GPIOx == GPIOF)
		{
			GPIOF_PCLK_EN();
		}
		else if(p_GPIOx == GPIOH)
		{
			GPIOH_PCLK_EN();
		}

	}else
	{
		if(p_GPIOx == GPIOA)
		{
			GPIOA_PCLK_DI();
		}
		else if(p_GPIOx == GPIOB)
		{
			GPIOB_PCLK_DI();
		}
		else if(p_GPIOx == GPIOC)
		{
			GPIOC_PCLK_DI();
		}
		else if(p_GPIOx == GPIOD)
		{
			GPIOD_PCLK_DI();
		}
		else if(p_GPIOx == GPIOE)
		{
			GPIOE_PCLK_DI();
		}
		else if(p_GPIOx == GPIOF)
		{
			GPIOF_PCLK_DI();
		}
		else if(p_GPIOx == GPIOH)
		{
			GPIOH_PCLK_DI();
		}
	}
}

/*
 * @func:			GPIO_init
 *
 * @brief:			This function initialize the given GPIO port
 *
 * @param[in]:		address of GPIO Handle
 *
 * @return: 		none
 *
 * @note: 			this function enables the peripheral clock
 */
void GPIO_init(GPIO_Handle_t *p_GPIO_Handle)
{
	//enable the peripheral clock
	GPIO_clock_control(p_GPIO_Handle->p_GPIOx, ENABLE);
	uint32_t temp;
	uint8_t mode = p_GPIO_Handle->GPIO_config.GPIO_pin_mode;
	uint8_t pin = p_GPIO_Handle->GPIO_config.GPIO_pin_num;
	//config mode
	if(mode <= GPIO_MODE_ANALOG)
	{
		temp = p_GPIO_Handle->p_GPIOx->MODER;	//read
		temp &= ~(0b11 << (2 * pin) );			//clear
		temp |= (mode << (2 * pin) );			//write to temp only
		p_GPIO_Handle->p_GPIOx->MODER = temp; 	//right back to the register
		temp = 0;
	}else{
		// Interrupt modes

		//configure as input first
		temp = p_GPIO_Handle->p_GPIOx->MODER;
		temp &= ~(0b11 << (2 * pin));  // 00 = input
		p_GPIO_Handle->p_GPIOx->MODER = temp;
		temp = 0;

		//configure edge trigger
		if(mode == GPIO_MODE_IT_FT)
		{
			//configure the FTSR
			EXTI->FTSR |= (1 << pin);
			//clear the corresponding RTSR bit
			EXTI->RTSR &= ~(1 << pin);
		}else if(mode == GPIO_MODE_IT_RT)
		{
			//configure the RTSR
			EXTI->RTSR |= (1 << pin);
			//clear the corresponding RTSR bit
			EXTI->FTSR &= ~(1 << pin);
		}else if (mode == GPIO_MODE_IT_RFT)
		{
			//configure both FTSR and RTSR
			EXTI->RTSR |= (1 << pin);
			EXTI->FTSR |= (1 << pin);
		}

		//configure the GPIO port selection in SYSCFG_EXTICRx
		uint8_t temp1 = pin / 4;
		uint8_t temp2 = pin % 4;
		uint8_t port_code = GPIO_BASEADDR_TO_CODE(p_GPIO_Handle->p_GPIOx);
		SYSCFG_PCLK_EN();
		SYSCFG->EXTICR[temp1] |= ( port_code<< (4 * temp2) );	//for example, set EXTI3 port to A port b/c using PA3 pin

		//enable the EXTI interrupt using IMR
		EXTI->IMR |= (1 << pin);
	}

	if(mode == GPIO_MODE_OUT || mode == GPIO_MODE_ALTFUNC)
	{
		//config output type
		temp = p_GPIO_Handle->p_GPIOx->OTYPER;
		temp &= ~(0b1 << pin );
		temp |= (p_GPIO_Handle->GPIO_config.GPIO_pin_out_type << pin );
		p_GPIO_Handle->p_GPIOx->OTYPER = temp;
		temp = 0;

		//config output speed
		temp = p_GPIO_Handle->p_GPIOx->OSPEEDR;
		temp &= ~(0b11 << (2 * pin) );
		temp |= (p_GPIO_Handle->GPIO_config.GPIO_pin_speed << (2 * pin) );
		p_GPIO_Handle->p_GPIOx->OSPEEDR = temp;
		temp = 0;
	}

	//config pull up/pull down settings
	temp = p_GPIO_Handle->p_GPIOx->PUPDR;
	temp &= ~(0b11 << (2 * pin) );
	temp |= (p_GPIO_Handle->GPIO_config.GPIO_pin_pupd << (2 * pin) );
	p_GPIO_Handle->p_GPIOx->PUPDR = temp;
	temp = 0;

	//config alt functionality
	if (mode == GPIO_MODE_ALTFUNC)
	{
		uint8_t temp1 = pin / 8;	//which register, 0(AFRL) or 1(AFRH)
		uint8_t temp2 = pin % 8;	//position within 0-7 for the pin in the corresponding register
		temp = p_GPIO_Handle->p_GPIOx->AFR[temp1];
		temp &= ~(0b1111 << (4 * temp2) );
		temp |= (p_GPIO_Handle->GPIO_config.GPIO_pin_alt_fcn_mode << (4 * temp2) );
		p_GPIO_Handle->p_GPIOx->AFR[temp1] = temp;
		temp = 0;
	}
}
/*
 * @func:			GPIO_deinit
 *
 * @brief:			This function resets all registers of the given GPIO port
 *
 * @param[in]:		base address of GPIO port registers
 *
 * @return: 		none
 */
void GPIO_deinit(GPIO_reg_t *p_GPIOx)
{
	// only need to set reset register to reset the GPIO port registers
	if(p_GPIOx == GPIOA)
	{
		GPIOA_REG_RESET();
	}
	else if(p_GPIOx == GPIOB)
	{
		GPIOB_REG_RESET();
	}
	else if(p_GPIOx == GPIOC)
	{
		GPIOC_REG_RESET();
	}
	else if(p_GPIOx == GPIOD)
	{
		GPIOD_REG_RESET();
	}
	else if(p_GPIOx == GPIOE)
	{
		GPIOE_REG_RESET();
	}
	else if(p_GPIOx == GPIOF)
	{
		GPIOF_REG_RESET();
	}
	else if(p_GPIOx == GPIOH)
	{
		GPIOH_REG_RESET();
	}
}

/*
 * @func:			GPIO_read_input_pin
 *
 * @brief:			This function reads the input value of the given GPIO pin
 *
 * @param[in]:		base address of GPIO port registers
 * @param[in]:		the specific pin number to read
 *
 * @return: 		the input value(0 or 1) of the pin
 */
uint8_t GPIO_read_input_pin(GPIO_reg_t *p_GPIOx, uint8_t pin_num)
{
	//shift to the right instead of shifting 1 to the left and do & bc that would not create 0 or 1 as output
	return (p_GPIOx->IDR >> pin_num) & 1;
}
/*
 * @func:			GPIO_read_input_port
 *
 * @brief:			This function reads the input value of the given GPIO port
 *
 * @param[in]:		base address of GPIO port registers
 *
 * @return: 		the input value of the 16 pins of the port
 */
uint16_t GPIO_read_input_port(GPIO_reg_t *p_GPIOx)
{
	return p_GPIOx->IDR;
}
/*
 * @func:			GPIO_write_output_pin
 *
 * @brief:			This function writes the given value to the given GPIO pin
 *
 * @param[in]:		base address of GPIO port registers
 * @param[in]:		pin number to write value to
 * @param[in]:		value to write to the pin
 *
 * @return: 		none
 */
void GPIO_write_output_pin(GPIO_reg_t *p_GPIOx, uint8_t pin_num, uint8_t val)
{
	if(val == SET)
	{
		p_GPIOx->ODR |= (1 << pin_num);
	}else
	{
		p_GPIOx->ODR &= ~(1 << pin_num);
	}
}
/*
 * @func:			GPIO_write_output_port
 *
 * @brief:			This function writes the given value to the given GPIO port
 *
 * @param[in]:		base address of GPIO port registers
 * @param[in]:		value to write to the port
 *
 * @return: 		none
 */
void GPIO_write_output_port(GPIO_reg_t *p_GPIOx, uint16_t val)
{
	p_GPIOx->ODR = val;
}
/*
 * @func:			GPIO_toggle_output_pin
 *
 * @brief:			This function toggles the given GPIO pin
 *
 * @param[in]:		base address of GPIO port registers
 * @param[in]:		pin number to toggle
 *
 * @return: 		none
 */
void GPIO_toggle_output_pin(GPIO_reg_t *p_GPIOx, uint8_t pin_num)
{
	p_GPIOx->ODR ^= (1 << pin_num);
}

/*
 * @func:			GPIO_IRQ_config
 *
 * @brief:			This function enable/disable interrupt for the given peripheral
 *
 * @param[in]:		the IRQ number to enable/disable
 * @param[in]:		ENABLE or DISABLE the IRQ
 *
 * @return: 		none
 */
void GPIO_IRQ_config(uint8_t IRQ_num, uint8_t enable)
{
	//enable the IRQ
	if(enable == ENABLE)
	{
		if(IRQ_num < 32)
		{
			//enable ISER0
			*NVIC_ISER0 |= (1 << IRQ_num);
		}else if(IRQ_num >= 32 && IRQ_num < 64)
		{
			//enable ISER1
			*NVIC_ISER1 |= (1 << (IRQ_num % 32));
		}else if(IRQ_num >= 64 && IRQ_num < 96){
			//enable ISER2
			*NVIC_ISER2 |= (1 << (IRQ_num % 64));
		}
		else if (IRQ_num >= 96 && IRQ_num < 128)
		{
			//enable ISER3
			*NVIC_ISER3 |= (1 << (IRQ_num % 96));
		}
	}else{ //disable the IRQ
		if(IRQ_num < 32)
		{
			//enable ICER0
			*NVIC_ICER0 |= (1 << IRQ_num);
		}else if(IRQ_num >= 32 && IRQ_num < 64)
		{
			//enable ICER1
			*NVIC_ICER1 |= (1 << (IRQ_num % 32));
		}else if(IRQ_num >= 64 && IRQ_num < 96){
			//enable ICER2
			*NVIC_ICER2 |= (1 << (IRQ_num % 64));
		}
		else if (IRQ_num >= 96 && IRQ_num < 128)
		{
			//enable ICER3
			*NVIC_ICER3 |= (1 << (IRQ_num % 96));
		}
	}

}

/*
 * @func:			GPIO_IRQ_config
 *
 * @brief:			This function enable/disable the GPIO pin as given
 *
 * @param[in]:		IRQ number of the peripheral to set priority
 * @param[in]:		priority value to set the IRQ to
 *
 * @return: 		none
 */
void GPIO_set_priority(uint8_t IRQ_num, uint8_t IRQ_priority)
{
	//set priority
	uint8_t iprx = IRQ_num / 4;						//which IRQ register, each IPR register only contain 4 interrupts (1 byte apart)
	uint8_t iprx_section = IRQ_num % 4;				//which interrupt(byte) within the IPR register
	uint8_t shift_amount = (8 * iprx_section) + 4; 	//add 4 because the upper 4 bits are the preemptive priority and the lower 4 are the subpriority

	*(NVIC_IPR_BASEADDR + iprx) |= (IRQ_priority << shift_amount); //NVIC_IPR_BASEADDR is uin32_t pointer so adding the iprx will be 4 bytes apart

}

/*
 * @func:			GPIO_IRQ_handler
 *
 * @brief:			This function clears the pending interrupt of the given pin
 *
 * @param[in]:		pin number to clear interrupt
 *
 * @return: 		none
 *
 * @note:			call in the EXTI handlers to clear the pending bit
 */
void GPIO_IRQ_handler(uint8_t pin_num)
{
	//check if corresponding bit in pending register is set
	if(EXTI->PR & (1 << pin_num))
	{
		//clear the pending bit (write 1 to clear)
		EXTI->PR |= (1 << pin_num);
	}
}

