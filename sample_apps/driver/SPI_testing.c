/*
 * SPI_testing.c
 *
 *  Created on: Dec 28, 2025
 *      Author: krisko
 */
#include <string.h>
#include "drivers.h"



/*
 * This sample application tests the SPI configuration, enable and send APIs
 *
 * pins used:
 * PB15 - SPI2_MOSI
 * PB14 - SPI2_MISO
 * PB13 - SPI2_SCLK
 * PB12 - SPI2_NSS
 * GPIO alternate function mode: 5
 *
 * note: MISO and NSS pins are disabled since there are no slave
 */

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
//	SPI_pins.GPIO_config.GPIO_pin_num = 12;
//	GPIO_init(&SPI_pins);
}

void SPI2_inits(void)
{
	SPI_Handle_t SPI2_handle;

	SPI2_handle.p_SPIx = SPI2;
	SPI2_handle.SPI_config.SPI_device_mode = SPI_DEVICE_MODE_MASTER;
	SPI2_handle.SPI_config.SPI_bus_config = SPI_BUS_CONFIG_FD; 	//full-duplex
	SPI2_handle.SPI_config.SPI_DFF = SPI_DFF_8BITS;
	SPI2_handle.SPI_config.SPI_sclk_speed = SPI_SCLK_SPEEDS_DIV2; //sclk speed is 8Mhz
	SPI2_handle.SPI_config.SPI_CPOL = SPI_CPOL_LOW;
	SPI2_handle.SPI_config.SPI_CPHA = SPI_CPHA_LEADING;
	SPI2_handle.SPI_config.SPI_SSM = SPI_SSM_EN;

	SPI_init(&SPI2_handle);
}

int main(void)
{
	//initialize the GPIO pins to behave as SPI2 pins
	SPI_GPIO_inits();
	//configure the SPI2 parameters
	SPI2_inits();
	//set SSI bit to 1 so NSS is 1 to avoid MODEF error, as a SSI bit = 0 in SSM mode will trigger MODEF flag
	SPI_SSI_config(SPI2, ENABLE);
	//enable SPI2
	SPI_periph_control(SPI2, ENABLE);

	char send_data[] = "SPI send";
	uint8_t data_len = strlen(send_data);
	//send data
	SPI_send(SPI2, (uint8_t*)send_data, data_len);
	//add disable and while bsy
	while( SPI_get_flag_status(SPI2, SPI_SR_BSY) );
	//disable SPI2
	SPI_SSI_config(SPI2, DISABLE);
	return 0;
}


