/*
 * SPI_driver.h
 *
 *  Created on: Dec 28, 2025
 *      Author: krisko
 */

#ifndef DRIVERS_SPI_DRIVER_H_
#define DRIVERS_SPI_DRIVER_H_

#include <stdint.h>
#include "STM32F446xx.h"

/*
 * SPI device configuration structure
 */
typedef struct
{
	uint8_t SPI_device_mode;	//!< possible values from @SPI_DEVICE_MODES
	uint8_t SPI_bus_config;		//!< possible values from @SPI_BUS_CONFIGS
	uint8_t SPI_sclk_speed;		//!< possible values from @SPI_SCLK_SPEEDS
	uint8_t SPI_DFF;			//!< possible values from @SPI_DFF
	uint8_t SPI_CPOL;			//!< possible values from @SPI_CPOL
	uint8_t SPI_CPHA;			//!< possible values from @SPI_CPHA
	uint8_t SPI_SSM;			//!< possible values from @SPI_SSM
}SPI_config_t;

/*
 * SPI device Handle structure
 */
typedef struct
{
	SPI_reg_t 		*p_SPIx;
	SPI_config_t 	SPI_config;
	uint8_t 		*p_Rx_buffer;
	uint8_t 		*p_Tx_buffer;
	uint32_t 		Tx_len;
	uint32_t 		Rx_len;
	uint8_t 		Rx_state;
	uint8_t 		Tx_state;
}SPI_Handle_t;


/*
 * @SPI_DEVICE_MODES
 */
#define SPI_DEVICE_MODE_SLAVE 	0
#define SPI_DEVICE_MODE_MASTER 	1

/*
 * @SPI_BUS_CONFIGS
 * note: for simplex Rx only(recieve only), just use full-duplex mode and disconnect the MISO line
 */
#define SPI_BUS_CONFIG_FD 			0		//full-duplex
#define SPI_BUS_CONFIG_HD 			1		//half-duplex
#define SPI_BUS_CONFIG_S_RXONLY		2		//simplex, Tx only(transmit only)

/*
 * @SPI_SCLK_SPEEDS
 */
#define SPI_SCLK_SPEEDS_DIV2		0
#define SPI_SCLK_SPEEDS_DIV4		1
#define SPI_SCLK_SPEEDS_DIV8		2
#define SPI_SCLK_SPEEDS_DIV16		3
#define SPI_SCLK_SPEEDS_DIV32		4
#define SPI_SCLK_SPEEDS_DIV64		5
#define SPI_SCLK_SPEEDS_DIV128		6
#define SPI_SCLK_SPEEDS_DIV256		7

/*
 * @SPI_DFF
 */
#define SPI_DFF_8BITS		0
#define SPI_DFF_16BITS		1

/*
 * @SPI_CPOL
 */
#define SPI_CPOL_LOW	0
#define SPI_CPOL_HIGH	1

/*
 * @SPI_CPHA
 */
#define SPI_CPHA_LEADING	0
#define SPI_CPHA_TRAILING	1

/*
 * @SPI_SSM
 */
#define SPI_SSM_DI	0		//disable
#define SPI_SSM_EN	1		//enable

/*
 * SPI peripheral states
 */
#define SPI_STATE_READY		0
#define SPI_STATE_BUSY_IN_RX		1
#define SPI_STATE_BUSY_IN_TX		2

/*
 * possible SPI application events
 */
#define SPI_EVENT_TX_CMPLT		0
#define SPI_EVENT_RX_CMPLT		1
#define SPI_EVENT_OVR_ERR		2


/**************************APIs**************************/

/*
 * clock setup
 */
void SPI_clock_control(SPI_reg_t *p_SPIx, uint8_t enable);


/*
 * initialize and diinitialize
 */
void SPI_init(SPI_Handle_t *p_SPI_Handle);
void SPI_deinit(SPI_reg_t *p_SPIx);



/*
 * send and receive
 */
void SPI_send(SPI_reg_t *p_SPIx, uint8_t *p_Tx_buffer, uint32_t len);
void SPI_recieve(SPI_reg_t *p_SPIx, uint8_t *p_Rx_buffer, uint32_t len);

/*
 * interrupt based send and receive
 */
uint8_t SPI_send_IT(SPI_Handle_t *p_SPI_Handle, uint8_t *p_Tx_buffer, uint32_t len);
uint8_t SPI_recieve_IT(SPI_Handle_t *p_SPI_Handle, uint8_t *p_Rx_buffer, uint32_t len);

/*
 * IQR configuration and handling
 */
void SPI_IRQ_config(uint8_t IRQ_num, uint8_t enable);
void SPI_set_priority(uint8_t IRQ_num, uint8_t IRQ_priority);
void SPI_IRQ_handler(SPI_Handle_t *p_SPI_Handle);

/*
 * other peripheral control APIs
 */
void SPI_periph_control(SPI_reg_t *p_SPIx, uint8_t enable);
uint8_t SPI_get_flag_status(SPI_reg_t *p_SPIx, uint8_t flag_bit);
void SPI_SSI_config(SPI_reg_t *p_SPIx, uint8_t enable);
void SPI_SSOE_config(SPI_reg_t *p_SPIx, uint8_t enable);
void SPI_clear_OVR_flag(SPI_Handle_t *p_SPI_Handle);
void SPI_close_transmission(SPI_Handle_t *p_SPI_Handle);
void SPI_close_reception(SPI_Handle_t *p_SPI_Handle);

/*
 * user application APIs
 */
__attribute__((weak)) void SPI_event_callback(SPI_Handle_t *p_SPI_Handle, uint8_t event);


#endif /* DRIVERS_SPI_DRIVER_H_ */
