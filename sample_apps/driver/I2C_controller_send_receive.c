/*
 * I2C_controller_send_receive.c
 *
 *  Created on: Dec 31, 2025
 *      Author: krisko
 */


#include <string.h>
#include <stdio.h>
#include "drivers.h"

/*
 * This sample application have STM32F446RE (controller) sending a code(0x67) to Arduino Uno (target) by I2C and Arduino sends back a
 * response to confirm the message length. Then controller sends another code(0x76) to Arduino, and Arduino sends the actual message
 * to STM32F446RE. STM32F446RE receives the the message and reads it into a buffer.
 *
 * pins used:
 * PB6 - I2C1_SCL
 * PB7 - I2C1_SDA
 * GPIO alternate function mode: 4
 *
 * connection with arduino:
 * PB8 (I2C1_SCL) - arduino pin A5
 * PB9 (I2C1_SDA) - arduino pin A4
 * STm32 GND - arduino GND
 */

#define BUTTON_PRESSED 	0
#define TARGET_ADDR 		0x68

I2C_Handle_t I2C1_Handle;

void I2C_GPIO_inits(void)
{
	GPIO_Handle_t I2C_pins;

	//pin configuration generic to all SPI pins
	I2C_pins.p_GPIOx = GPIOB;
	I2C_pins.GPIO_config.GPIO_pin_mode = GPIO_MODE_ALTFUNC;
	I2C_pins.GPIO_config.GPIO_pin_alt_fcn_mode = 4;
	I2C_pins.GPIO_config.GPIO_pin_out_type = GPIO_OUT_TYPE_OD;
	I2C_pins.GPIO_config.GPIO_pin_speed = GPIO_OUT_SPEED_FAST;
	I2C_pins.GPIO_config.GPIO_pin_pupd = GPIO_PIN_PU;

	//SCL
	I2C_pins.GPIO_config.GPIO_pin_num = GPIO_PIN_NO_8;
	GPIO_init(&I2C_pins);

	//SDA
	I2C_pins.GPIO_config.GPIO_pin_num = GPIO_PIN_NO_9;
	GPIO_init(&I2C_pins);
}

void GPIO_button_init(void)
{
	GPIO_Handle_t GPIO_button;
	//button configuration
	GPIO_button.p_GPIOx = GPIOC; 	//B1 is connected to PC13
	GPIO_button.GPIO_config.GPIO_pin_num = GPIO_PIN_NO_13;
	GPIO_button.GPIO_config.GPIO_pin_mode = GPIO_MODE_IN;
	GPIO_button.GPIO_config.GPIO_pin_pupd = GPIO_PIN_NO_PUPD;
	GPIO_init(&GPIO_button);
}

void delay(void)
{
	for(uint32_t i = 0; i < 500000; i++);
}

void I2C1_inits(void)
{
	I2C1_Handle.p_I2Cx = I2C1;
	I2C1_Handle.I2Cx_config.I2C_ACK_control = I2C_ACK_ENABLE;
	I2C1_Handle.I2Cx_config.I2C_CLK_speed = I2C_CLK_SPEED_SM;
	I2C1_Handle.I2Cx_config.I2C_FM_duty_cycle = I2C_FM_DUTY_2;
	I2C1_Handle.I2Cx_config.I2C_device_addr = 0x67;

	I2C_init(&I2C1_Handle);
}

int main(void)
{
	setvbuf(stdout, NULL, _IONBF, 0); //disable buffering
	printf("Application started\n");
	//initialize the GPIO pins to behave as I2C1 pins
	I2C_GPIO_inits();
	//initialize the B1 button
	GPIO_button_init();
	//configure the I2C1 parameters
	I2C1_inits();
	//enable I2C1
	I2C_periph_control(&I2C1_Handle, ENABLE);


	uint8_t received_mssg[32], len, command_code;
	while(1)
	{
		while(GPIO_read_input_pin(GPIOC, GPIO_PIN_NO_13));
		delay();


		//get length information from target
		command_code = 0x67;
		I2C_controller_send(&I2C1_Handle, &command_code, 1, TARGET_ADDR, I2C_RS_ENABLE);
		I2C_controller_receive(&I2C1_Handle, &len, 1, TARGET_ADDR, I2C_RS_ENABLE);
		printf("Receiving message of length: %d\n", len);

		//get the message from target
		command_code = 0x76;
		I2C_controller_send(&I2C1_Handle, &command_code, 1, TARGET_ADDR, I2C_RS_ENABLE);
		I2C_controller_receive(&I2C1_Handle, received_mssg, len, TARGET_ADDR, I2C_RS_DISABLE);

		received_mssg[len] = '\0';

		printf("Received data: %s\n", received_mssg);
	}




	return 0;
}

