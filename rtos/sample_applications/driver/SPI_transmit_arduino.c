/*
 * SPI_transmit_arduino.c
 *
 *  Created on: Dec 29, 2025
 *      Author: krisko
 */
#include <string.h>
#include "drivers.h"

/*
 * This sample application transmits a message to Arduino Uno using SPI driver when B1 button on STM32F446RE is pressed
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
 * STm32 GND - arduino GND
 * PB14 (SPI2_MISO) not connected
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
//	SPI_pins.GPIO_config.GPIO_pin_num = 14;
//	GPIO_init(&SPI_pins);

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


	char send_data[] = "Testing SPI send";
	uint8_t data_len = strlen(send_data);

	while(1)
	{
		// Wait for button press
		while(GPIO_read_input_pin(GPIOC, GPIO_PIN_NO_13) != BUTTON_PRESSED);
		delay();  // avoid debounce press

		//toggle LED
		GPIO_toggle_output_pin(GPIOA, GPIO_PIN_NO_5);

		//enable SPI2
		SPI_periph_control(SPI2, ENABLE);

		//first send the length
		SPI_send(SPI2, &data_len, 1);
		while( SPI_get_flag_status(SPI2, SPI_SR_BSY) );

		//send data
		SPI_send(SPI2, (uint8_t*)send_data, data_len);

		//wait until last byte is transmitted successfully, that is BSY bits turns 0
		while( SPI_get_flag_status(SPI2, SPI_SR_BSY) );

		//disable SPI2 so NSS is pulled to 1
		SPI_periph_control(SPI2, DISABLE);

		//toggle LED
		GPIO_toggle_output_pin(GPIOA, GPIO_PIN_NO_5);
		delay();
	}

	return 0;
}



