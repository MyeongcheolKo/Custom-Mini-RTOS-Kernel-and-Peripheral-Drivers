/*
 * USART_driver.h
 *
 *  Created on: Jan 3, 2026
 *      Author: krisko
 */

#ifndef DRIVERS_USART_DRIVER_H_
#define DRIVERS_USART_DRIVER_H_

#include "STM32F446xx.h"
#include "rcc.h"

typedef struct
{
	uint8_t 	USART_mode;					//!< possible values from @USART_MODE
	uint32_t 	USART_baud;					//!< possible values from @USART_BAUD
	uint8_t 	USART_num_stop_bits;		//!< possible values from @USART_NUM_STOP_BITS
	uint8_t		USART_word_len;				//!< possible values from @USART_WORD_LEN
	uint8_t 	USART_parity;				//!< possible values from @USART_PARITY
	uint8_t 	USART_HW_flow_ctrl;			//!< possible values from @USART_HW_FLOW_CTRL
}USART_config_t;

typedef struct
{
	USART_reg_t 	*p_USARTx;
	USART_config_t	USARTx_config;
	uint8_t			*p_Tx_buffer;
	uint8_t			*p_Rx_buffer;
	uint32_t 		Tx_len;
	uint32_t 		Rx_len;
	uint8_t 		Tx_state;
	uint8_t 		Rx_state;
}USART_Handle_t;

/*
 *@USART_MODE
 */
#define USART_MODE_ONLY_TX 0
#define USART_MODE_ONLY_RX 1
#define USART_MODE_TXRX  2

/*
 *@USART_BAUD
 */
#define USART_STD_BAUD_1200					1200
#define USART_STD_BAUD_2400					2400
#define USART_STD_BAUD_9600					9600
#define USART_STD_BAUD_19200 				19200
#define USART_STD_BAUD_38400 				38400
#define USART_STD_BAUD_57600 				57600
#define USART_STD_BAUD_115200 				115200
#define USART_STD_BAUD_230400 				230400
#define USART_STD_BAUD_460800 				460800
#define USART_STD_BAUD_921600 				921600
#define USART_STD_BAUD_2M 					2000000
#define SUART_STD_BAUD_3M 					3000000

/*
 *@USART_NUM_STOP_BITS
 */
#define USART_STOP_BITS_1     0
#define USART_STOP_BITS_0_5   1
#define USART_STOP_BITS_2     2
#define USART_STOP_BITS_1_5   3

/*
 *@USART_WORD_LEN
 */
#define USART_WORD_LEN_8BITS  0
#define USART_WORD_LEN_9BITS  1

/*
 *@USART_PARITY
 */
#define USART_PARITY_DISABLE  0
#define USART_PARITY_EN_EVEN  1
#define USART_PARITY_EN_ODD   2


/*
 *@USART_HW_FLOW_CTRL
 */
#define USART_HW_FLOW_CTRL_NONE    	0
#define USART_HW_FLOW_CTRL_CTS    	1
#define USART_HW_FLOW_CTRL_RTS    	2
#define USART_HW_FLOW_CTRL_CTS_RTS	3

/*
 * possible USART peripheral states
 */
#define USART_STATE_READY		0
#define USART_STATE_BUSY_TX		1
#define USART_STATE_BUSY_RX		2

/*
 * possible user application callback events
 */
#define USART_EV_TX_CMPLT		0
#define USART_EV_RX_CMPLT		1
#define USART_EV_CTS			2
#define USART_EV_IDLE			3
#define USART_EV_ORE			4
#define USART_ER_NF				5
#define USART_ER_FE				6


/**************************APIs**************************/

/*
 * clock setup
 */
void USART_clock_control(USART_reg_t *p_USARTx, uint8_t enable);

/*
 * initialize and diinitialize
 */
void USART_init(USART_Handle_t *p_USART_Handle);
void USART_deinit(USART_reg_t *p_USARTx);

/*
 * send and receive
 */
void USART_send(USART_Handle_t *p_USART_Handle, uint8_t *p_Tx_buffer, uint32_t len);
void USART_receive(USART_Handle_t *p_USART_Handle, uint8_t *p_Rx_buffer, uint32_t len);

/*
 * Interrupt based send and receive
 */
uint8_t USART_send_IT(USART_Handle_t *p_USART_Handle, uint8_t *p_Tx_buffer, uint32_t len);
uint8_t USART_receive_IT(USART_Handle_t *p_USART_Handle, uint8_t *p_Rx_buffer, uint32_t len);

/*
 * IQR configuration and handling
 */
void USART_IRQ_config(uint8_t IRQ_num, uint8_t enable);
void USART_set_priority(uint8_t IRQ_num, uint8_t IRQ_priority);
void USART_IRQ_handling(USART_Handle_t *p_USART_Handle);

/*
 * other peripheral control APIs
 */
void USART_periph_control(USART_Handle_t *p_USART_Handle, uint8_t enable);
uint8_t USART_get_flag_status(USART_reg_t *p_USARTx, uint8_t flag_bit);
void USART_close_send(USART_Handle_t *p_USART_Handle);
void USART_close_receive(USART_Handle_t *p_USART_Handle);

/*
 * user application APIs
 */
__attribute__((weak)) void USART_event_callback(USART_Handle_t *p_USART_Handle, uint8_t event);

#endif /* DRIVERS_USART_DRIVER_H_ */
