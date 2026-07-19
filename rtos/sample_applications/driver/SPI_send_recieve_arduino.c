/*
 * SPI_send_recieve_arduino.c
 *
 *  Created on: Dec 29, 2025
 *      Author: krisko
 */

#include <string.h>
#include "drivers.h"

/*
 * This sample application have STM32F446RE (Master) sending a command byte (67) to Arduino Uno (Slave), and
 * Arduino responds with 0xF5 as acknowledgement if it verifies it receives 67, the correct command.
 * STM32 then blinks LD2 LED if it receives the valid acknowledgement (0xF5).
 *
 * pins used:
 * PB15 - SPI2_MOSI
 * PB14 - SPI2_MISO
 * PB13 - SPI2_SCLK
 * PB12 - SPI2_NSS
 * GPIO alternate function mode: 5
 *
 * connection with arduino:
 * STM32 (master), Arduino (slave)
 * PB15 (SPI2_MOSI) - arduino pin 11
 * PB13 (SPI2_SCLK) - arduino pin 13
 * PB12 (SPI2_NSS) - arduino pin 10
 * PB14 (SPI2_MISO) - arduino pin 12
 * STm32 GND - arduino GND

 *
 * note: use a logic level converter to avoid data corruption due to VOH and VIH mismatch
 */

#define BUTTON_PRESSED 0

void SPI_GPIO_inits(void)
{
	GPIO_Handle_t SPI_pins;

	//pin configuration generic to all SPI pins
	SPI_pins.p_GPIOx = GPIOB;
	SPI_pins.GPIO_config.GPIO_pin_mode = GPIO_MODE_ALTFUNC;
	SPI_pins.GPIO_config.GPIO_pin_alt_fcn_mode = 5;
	SPI_pins.GPIO_config.GPIO_pin_out_type = GPIO_OUT_TYPE_PP;
	SPI_pins.GPIO_config.GPIO_pin_speed = GPIO_OUT_SPEED_FAST;
	SPI_pins.GPIO_config.GPIO_pin_pupd = GPIO_PIN_NO_PUPD;

	//MOSI
	SPI_pins.GPIO_config.GPIO_pin_num = 15;
	GPIO_init(&SPI_pins);

	//MISO
	SPI_pins.GPIO_config.GPIO_pin_num = 14;
	GPIO_init(&SPI_pins);

	//SCLK
	SPI_pins.GPIO_config.GPIO_pin_num = 13;
	GPIO_init(&SPI_pins);

	//NSS
	SPI_pins.GPIO_config.GPIO_pin_num = 12;
	GPIO_init(&SPI_pins);

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

void GPIO_LED_init(void)
{
	GPIO_Handle_t GPIO_led;
	//LED configuration
	GPIO_led.p_GPIOx = GPIOA; 	//LD2 is connected to PA5
	GPIO_led.GPIO_config.GPIO_pin_num = GPIO_PIN_NO_5;
	GPIO_led.GPIO_config.GPIO_pin_mode = GPIO_MODE_OUT;
	GPIO_led.GPIO_config.GPIO_pin_speed = GPIO_OUT_SPEED_FAST;
	GPIO_led.GPIO_config.GPIO_pin_out_type = GPIO_OUT_TYPE_PP;
	GPIO_led.GPIO_config.GPIO_pin_pupd = GPIO_PIN_NO_PUPD;
	GPIO_init(&GPIO_led);
}

void delay(void)
{
	for(uint32_t i = 0; i < 500000; i++);
}

void SPI2_inits(void)
{
	SPI_Handle_t SPI2_handle;

	SPI2_handle.p_SPIx = SPI2;
	SPI2_handle.SPI_config.SPI_device_mode = SPI_DEVICE_MODE_MASTER;
	SPI2_handle.SPI_config.SPI_bus_config = SPI_BUS_CONFIG_FD; 	//full-duplex
	SPI2_handle.SPI_config.SPI_DFF = SPI_DFF_8BITS;
	SPI2_handle.SPI_config.SPI_sclk_speed = SPI_SCLK_SPEEDS_DIV64;
	SPI2_handle.SPI_config.SPI_CPOL = SPI_CPOL_LOW;
	SPI2_handle.SPI_config.SPI_CPHA = SPI_CPHA_LEADING;
	SPI2_handle.SPI_config.SPI_SSM = SPI_SSM_DI;

	SPI_init(&SPI2_handle);
}

uint8_t SPI_verify_response(uint8_t ack_byte)
{
	if(ack_byte == 0xF5)
	{
		return 1;
	}
	return 0;
}

int main(void)
{
	//initialize the GPIO pins to behave as SPI2 pins
	SPI_GPIO_inits();
	//initialize the B1 button
	GPIO_button_init();
	//initialize the L2 LED
	GPIO_LED_init();
	//configure the SPI2 parameters
	SPI2_inits();
	//enable SSOE bit so NSS is pulled to 0 automatically when SPE is set
	SPI_SSOE_config(SPI2, ENABLE);


	uint8_t send_data = 67;
	uint8_t dummy_byte;
	uint8_t response;

	while(1)
	{
		// Wait for button press
		while(GPIO_read_input_pin(GPIOC, GPIO_PIN_NO_13) != BUTTON_PRESSED);
		delay();  // avoid debounce


		//enable SPI2
		SPI_periph_control(SPI2, ENABLE);

		//send data
		SPI_send(SPI2, &send_data, 1);
		while( SPI_get_flag_status(SPI2, SPI_SR_BSY) );

		//read the Rx buffer to clear RXNE, to avoid data lost
		SPI_recieve(SPI2, &dummy_byte, 1);
		//send some dummy byte to fetch the response from the slave
		dummy_byte = 0xFF;
		SPI_send(SPI2, &dummy_byte, 1);
		while( SPI_get_flag_status(SPI2, SPI_SR_BSY) );

		//read the response
		SPI_recieve(SPI2, &response, 1);

		//check response
		if(SPI_verify_response(response)){
			//toggle LED
			GPIO_toggle_output_pin(GPIOA, GPIO_PIN_NO_5);
			delay();
			GPIO_toggle_output_pin(GPIOA, GPIO_PIN_NO_5);
		}
		//disable SPI2 so NSS is pulled to 1
		SPI_periph_control(SPI2, DISABLE);

	}

	return 0;
}



