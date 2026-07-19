/*
 * LED_Toggle.c
 *
 *  Created on: Dec 27, 2025
 *      Author: krisko
 */

#include "GPIO_driver.h"

void delay(void);

/*
 * This sample application toggles the onboard LD2 on the nucleo STM32F446RE board using the GPIO driver
 */
int main(void)
{
	GPIO_Handle_t GPIO_led;

	GPIO_led.p_GPIOx = GPIOA; 	//LD2 is connected to PA5
	GPIO_led.GPIO_config.GPIO_pin_num = GPIO_PIN_NO_5;
	GPIO_led.GPIO_config.GPIO_pin_mode = GPIO_MODE_OUT;
	GPIO_led.GPIO_config.GPIO_pin_speed = GPIO_OUT_SPEED_FAST;

	//push-pull with no pull up or pull down
	GPIO_led.GPIO_config.GPIO_pin_out_type = GPIO_OUT_TYPE_PP;
	GPIO_led.GPIO_config.GPIO_pin_pupd = GPIO_PIN_NO_PUPD;

	//open drain with pull up, led should be toggling but with very very small intensity
	//need external pull up resistor for the LED to light up with normal intensity
	//for example, connect PA5 to 5V with an external resistor
//	GPIO_led.GPIO_config.GPIO_pin_out_type = GPIO_OUT_TYPE_OD;
//	GPIO_led.GPIO_config.GPIO_pin_pupd = GPIO_PIN_PU;

	GPIO_clock_control(GPIO_led.p_GPIOx, ENABLE);
	GPIO_init(&GPIO_led);

	GPIO_toggle_output_pin(GPIO_led.p_GPIOx,GPIO_led.GPIO_config.GPIO_pin_num);


	while(1){
		GPIO_toggle_output_pin(GPIO_led.p_GPIOx,GPIO_led.GPIO_config.GPIO_pin_num);
		delay();
	}
	return 0;
}

void delay(void)
{
	for(uint32_t i = 0; i < 500000; i++);
}

