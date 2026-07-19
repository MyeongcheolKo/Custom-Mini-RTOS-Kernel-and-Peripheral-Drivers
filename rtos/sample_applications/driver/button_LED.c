/*
 * button_LED.c
 *
 *  Created on: Dec 27, 2025
 *      Author: krisko
 */


#include "GPIO_driver.h"

#define BUTTON_PRESSED 0

void delay(void);

/*
 * This sample application toggles the onboard LD2 on the nucleo STM32F446RE when the
 * onboard button B1 is pressed using the GPIO driver
 */
int main(void)
{
	GPIO_Handle_t GPIO_led;
	GPIO_Handle_t GPIO_button;

	//LED configuration
	GPIO_led.p_GPIOx = GPIOA; 	//LD2 is connected to PA5
	GPIO_led.GPIO_config.GPIO_pin_num = GPIO_PIN_NO_5;
	GPIO_led.GPIO_config.GPIO_pin_mode = GPIO_MODE_OUT;
	GPIO_led.GPIO_config.GPIO_pin_speed = GPIO_OUT_SPEED_FAST;
	GPIO_led.GPIO_config.GPIO_pin_out_type = GPIO_OUT_TYPE_PP;
	GPIO_led.GPIO_config.GPIO_pin_pupd = GPIO_PIN_NO_PUPD;
	GPIO_init(&GPIO_led);

	//button configuration
	GPIO_button.p_GPIOx = GPIOC; 	//B1 is connected to PC13
	GPIO_button.GPIO_config.GPIO_pin_num = GPIO_PIN_NO_13;
	GPIO_button.GPIO_config.GPIO_pin_mode = GPIO_MODE_IN;
	GPIO_button.GPIO_config.GPIO_pin_pupd = GPIO_PIN_NO_PUPD;
	GPIO_init(&GPIO_button);


	while(1){
		if (GPIO_read_input_pin(GPIO_button.p_GPIOx, GPIO_button.GPIO_config.GPIO_pin_num) == BUTTON_PRESSED)
		{
			delay();
			GPIO_toggle_output_pin(GPIO_led.p_GPIOx,GPIO_led.GPIO_config.GPIO_pin_num);
		}

	}
	return 0;
}

void delay(void)
{
	for(uint32_t i = 0; i < 250000; i++);
}
