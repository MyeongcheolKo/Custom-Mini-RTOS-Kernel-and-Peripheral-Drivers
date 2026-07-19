/*
 * external_button_LED.c
 *
 *  Created on: Dec 27, 2025
 *      Author: krisko
 */

#include "GPIO_driver.h"

#define BUTTON_PRESSED 0

void delay(void);

/*
 * This sample application toggles the external LD2 when an external button is pressed using STM32F446RE and the GPIO driver
 */
int main(void)
{
	GPIO_Handle_t GPIO_led;
	GPIO_Handle_t GPIO_button;

	//LED configuration
	GPIO_led.p_GPIOx = GPIOA; 	//LED is connected to PA10
	GPIO_led.GPIO_config.GPIO_pin_num = GPIO_PIN_NO_10;
	GPIO_led.GPIO_config.GPIO_pin_mode = GPIO_MODE_OUT;
	GPIO_led.GPIO_config.GPIO_pin_speed = GPIO_OUT_SPEED_FAST;
	GPIO_led.GPIO_config.GPIO_pin_out_type = GPIO_OUT_TYPE_PP;
	GPIO_led.GPIO_config.GPIO_pin_pupd = GPIO_PIN_NO_PUPD;
	GPIO_clock_control(GPIO_led.p_GPIOx, ENABLE);
	GPIO_init(&GPIO_led);

	//button configuration
	GPIO_button.p_GPIOx = GPIOB; 	//button is connected to PB5
	GPIO_button.GPIO_config.GPIO_pin_num = GPIO_PIN_NO_5;
	GPIO_button.GPIO_config.GPIO_pin_mode = GPIO_MODE_IT_FT;	//falling edge trigger
	GPIO_button.GPIO_config.GPIO_pin_pupd = GPIO_PIN_PU;
	GPIO_clock_control(GPIO_button.p_GPIOx, ENABLE);
	GPIO_init(&GPIO_button);

	//interrupt configs
	GPIO_set_priority(IRQ_NO_EXTI9_5, 10);
	//enable the corresponding EXTI
	GPIO_IRQ_config(IRQ_NO_EXTI9_5, ENABLE); 	// button is PB5 so EXTI5

	while(1){
	}
	return 0;
}

void EXTI9_5_IRQHandler(void)
{
	delay();
	GPIO_toggle_output_pin(GPIOA, GPIO_PIN_NO_10);
	GPIO_IRQ_handler(GPIO_PIN_NO_5);

}

void delay(void)
{
	for(uint32_t i = 0; i < 250000; i++);
}

