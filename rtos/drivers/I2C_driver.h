/*
 * I2C_driver.h
 *
 *  Created on: Dec 30, 2025
 *      Author: krisko
 */

#ifndef DRIVERS_I2C_DRIVER_H_
#define DRIVERS_I2C_DRIVER_H_

#include "STM32F446xx.h"
#include "rcc.h"

/*
 * I2C configuration structure
 */
typedef struct
{
	uint32_t 	I2C_CLK_speed;				//!< possible values from @I2C_CLK_SPEED
	uint8_t 	I2C_device_addr;			//values specified by user application
	uint8_t 	I2C_ACK_control;			//!< possible values from @I2C_ACK_CONTROL
	uint16_t	I2C_FM_duty_cycle;			//!< possible values from @I2C_FM_DUTY
}I2C_config_t;

/*
 * Handle structure for I2C peripheral
 */
typedef struct
{
	I2C_reg_t 		*p_I2Cx;
	I2C_config_t	I2Cx_config;
	uint8_t 		*p_Tx_buffer;
	uint8_t 		*p_Rx_buffer;
	uint32_t		Tx_len;
	uint32_t 		Rx_len;				//remaining bytes to receive
	uint8_t 		TxRxstate;
	uint8_t 		target_addr;
	uint32_t 		Rx_size;			//total bytes to receive
	uint8_t 		repeated_start;
}I2C_Handle_t;

/*
 * @I2C_CLK_SPEED
 */
#define I2C_CLK_SPEED_SM		100000	//standard mode
#define I2C_CLK_SPEED_FM2K		200000	//fast mode 2k
#define I2C_CLK_SPEED_FM4K		400000	//fast mode 4k

/*
 * @I2C_ACK_CONTROL
 */
#define I2C_ACK_DISABlE		0
#define I2C_ACK_ENABLE		1


/*
 *  @I2C_FM_DUTY
 */
#define I2C_FM_DUTY_2		0
#define I2C_FM_DUTY_16_9	1

/*
 * possible I2C states
 */
#define I2C_STATE_READY		0
#define I2C_STATE_BUSY_TX	1
#define I2C_STATE_BUSY_RX	2

/*
 * possible repeated start(RS) values
 */
#define I2C_RS_DISABLE	0
#define I2C_RS_ENABLE	1

/*
 * I2C application event macros
 */
#define I2C_EV_TX_CMPLT		0
#define I2C_EV_RX_CMPLT		1
#define I2C_EV_STOP			2
#define I2C_ER_BERR			3
#define I2C_ER_ARLO			4
#define I2C_ER_AF			5
#define I2C_ER_OVR			6
#define I2C_ER_PECERR		7
#define I2C_ER_TIMEOUT		8
#define I2C_ER_SMBALERT		9
#define I2C_EV_DATA_REQ		10
#define I2C_EV_DATA_REC		11



/**************************APIs**************************/

/*
 * clock setup
 */
void I2C_clock_control(I2C_reg_t *p_I2Cx, uint8_t enable);

/*
 * initialize and diinitialize
 */
void I2C_init(I2C_Handle_t *p_I2C_Handle);
void I2C_deinit(I2C_reg_t *p_I2Cx);

/*
 * send and receive
 */
void I2C_controller_send(I2C_Handle_t *p_I2C_Handle, uint8_t *p_Tx_buffer, uint32_t len, uint8_t target_addr, uint8_t RS_enable);
void I2C_controller_receive(I2C_Handle_t *p_I2C_Handle, uint8_t *p_Rx_buffer, uint32_t len, uint8_t target_addr, uint8_t RS_enable);
void I2C_target_send(I2C_reg_t *p_I2Cx, uint8_t data);
uint8_t I2C_target_receive(I2C_reg_t *p_I2Cx);


/*
 * Interrupt based send and receive
 */
uint8_t I2C_controller_send_IT(I2C_Handle_t *p_I2C_Handle, uint8_t *p_Tx_buffer, uint32_t len, uint8_t target_addr, uint8_t RS_enable);
uint8_t I2C_controller_receive_IT(I2C_Handle_t *p_I2C_Handle, uint8_t *p_Rx_buffer, uint32_t len, uint8_t target_addr, uint8_t RS_enable);

/*
 * IQR configuration and handling
 */
void I2C_IRQ_config(uint8_t IRQ_num, uint8_t enable);
void I2C_set_priority(uint8_t IRQ_num, uint8_t IRQ_priority);
void I2C_EV_IRQ_handling(I2C_Handle_t *p_I2C_Handle);
void I2C_ER_IRQ_handling(I2C_Handle_t *p_I2C_Handle);

/*
 * other peripheral control APIs
 */
void I2C_periph_control(I2C_Handle_t *p_I2C_Handle, uint8_t enable);
uint8_t I2C_get_flag_status(I2C_reg_t *p_I2Cx, uint8_t SR, uint8_t flag_bit);
void I2C_manage_acking(I2C_Handle_t *p_I2C_Handle, uint8_t enable);
void I2C_close_send(I2C_Handle_t *p_I2C_Handle);
void I2C_close_receive(I2C_Handle_t *p_I2C_Handle);
void I2C_generate_stop(I2C_Handle_t *p_I2C_Handle);

/*
 * user application APIs
 */
__attribute__((weak)) void I2C_event_callback(I2C_Handle_t *p_I2C_Handle, uint8_t event);



#endif /* DRIVERS_I2C_DRIVER_H_ */
