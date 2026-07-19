/*
 * GPIO_driver.h
 *
 *  Created on: Dec 26, 2025
 *      Author: krisko
 */

#ifndef DRIVERS_GPIO_DRIVER_H_
#define DRIVERS_GPIO_DRIVER_H_

#include "STM32F446xx.h"

/*
 * GPIO pin configuration structure
 */
typedef struct
{
	uint8_t GPIO_pin_num;				//!< possible values from @GPIO_PIN_NUMBERS
	uint8_t GPIO_pin_mode;				//!< possible values from @GPIO_PIN_MODES
	uint8_t GPIO_pin_speed;				//!< possible values from @GPIO_PIN_SPEEDS
	uint8_t GPIO_pin_pupd;				//!< possible values from @GPIO_PUPD
	uint8_t GPIO_pin_out_type;			//!< possible values from @GPIO_OUT_TYPES
	uint8_t GPIO_pin_alt_fcn_mode;
}GPIO_pin_config_t;

/*
 * Handle structure for a GPIO pin
 */
typedef struct
{
	GPIO_reg_t *p_GPIOx; 			//!<the specific GPIO port base address specified by the user
	GPIO_pin_config_t GPIO_config; 	//!< the GPIO configuration settings specified by user
}GPIO_Handle_t;

/*
 * @GPIO_PIN_NUMBERS
 */
#define GPIO_PIN_NO_0	0
#define GPIO_PIN_NO_1	1
#define GPIO_PIN_NO_2	2
#define GPIO_PIN_NO_3	3
#define GPIO_PIN_NO_4	4
#define GPIO_PIN_NO_5	5
#define GPIO_PIN_NO_6	6
#define GPIO_PIN_NO_7	7
#define GPIO_PIN_NO_8	8
#define GPIO_PIN_NO_9	9
#define GPIO_PIN_NO_10	10
#define GPIO_PIN_NO_11	11
#define GPIO_PIN_NO_12	12
#define GPIO_PIN_NO_13	13
#define GPIO_PIN_NO_14	14
#define GPIO_PIN_NO_15	15

/*
 * @GPIO_PIN_MODES
 * GPIO pin possible modes
 */
//non-interrupt modes
#define GPIO_MODE_IN 		0	//0b00
#define GPIO_MODE_OUT 		1	//0b01
#define GPIO_MODE_ALTFUNC 	2	//0b10
#define GPIO_MODE_ANALOG 	3	//0b11
//interrupt modes
#define GPIO_MODE_IT_FT 	4 	//falling edge
#define GPIO_MODE_IT_RT 	5	//rising edge
#define GPIO_MODE_IT_RFT 	6	//rising and falling

/*
 * @GPIO_OUT_TYPES
 * GPIO pin possible output types
 */
#define GPIO_OUT_TYPE_PP 0 		//push pull
#define GPIO_OUT_TYPE_OD 1 		//open drain

/*
 * @GPIO_PIN_SPEEDS
 * GPIO pin possible output speeds
 */
#define GPIO_OUT_SPEED_LOW		0
#define GPIO_OUT_SPEED_MEDIUM	1
#define GPIO_OUT_SPEED_FAST		2
#define GPIO_OUT_SPEED_HIGH		3

/*
 * @GPIO_PUPD
 * GPIO pin pull up and pull down config
 */
#define GPIO_PIN_NO_PUPD	0 	//no pull up nor pull down
#define GPIO_PIN_PU			1	//pull up
#define GPIO_PIN_PD			2	//pull down



/**************************APIs**************************/

/*
 * clock setup
 */
void GPIO_clock_control(GPIO_reg_t *p_GPIOx, uint8_t enable);


/*
 * initialize and diinitialize
 */
void GPIO_init(GPIO_Handle_t *p_GPIO_Handle);
void GPIO_deinit(GPIO_reg_t *p_GPIOx);



/*
 * read and write
 */
uint8_t GPIO_read_input_pin(GPIO_reg_t *p_GPIOx, uint8_t pin_num);
uint16_t GPIO_read_input_port(GPIO_reg_t *p_GPIOx);\

void GPIO_write_output_pin(GPIO_reg_t *p_GPIOx, uint8_t pin_num, uint8_t val);
void GPIO_write_output_port(GPIO_reg_t *p_GPIOx, uint16_t val);
void GPIO_toggle_output_pin(GPIO_reg_t *p_GPIOx, uint8_t pin_num);


/*
 * IQR configuration and handling
 */
void GPIO_IRQ_config(uint8_t IRQ_num, uint8_t enable);
void GPIO_set_priority(uint8_t IRQ_num, uint8_t IRQ_priority);
void GPIO_IRQ_handler(uint8_t pin_num);


#endif /* DRIVERS_GPIO_DRIVER_H_ */
