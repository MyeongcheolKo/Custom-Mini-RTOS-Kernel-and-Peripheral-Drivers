/*
 * I2C_driver.c
 *
 *  Created on: Dec 30, 2025
 *      Author: krisko
 */

#include "I2C_driver.h"
//private helper functions
static void I2C_generate_start(I2C_Handle_t *p_I2C_Handle);
static void I2C_execute_addr_phase(I2C_Handle_t *p_I2C_Handle, uint8_t target_addr, uint8_t read_or_write);
static void I2C_clear_ADDR_flag(I2C_Handle_t *p_I2C_Handle);
static void I2C_controller_RXNE_handler(I2C_Handle_t *p_I2C_Handle);
static void I2C_controller_TXE_handler(I2C_Handle_t *p_I2C_Handle);

/*
 * @func:			I2C_clock_control
 *
 * @brief:			This function enable/disable the clock for the given I2C peripheral
 *
 * @param[in]:		address of I2C peripheral
 * @param[in]:		ENABLE or DISABLE
 *
 * @return:			none
 */
void I2C_clock_control(I2C_reg_t *p_I2Cx, uint8_t enable)
{
	if(enable == ENABLE)
	{
		if(p_I2Cx == I2C1)
		{
			I2C1_PCLK_EN();
		}
		else if(p_I2Cx == I2C2)
		{
			I2C2_PCLK_EN();
		}
		else if(p_I2Cx == I2C3)
		{
			I2C3_PCLK_EN();
		}
	}else
	{
		if(p_I2Cx == I2C1)
		{
			I2C1_PCLK_DI();
		}
		else if(p_I2Cx == I2C2)
		{
			I2C2_PCLK_DI();
		}
		else if(p_I2Cx == I2C3)
		{
			I2C3_PCLK_DI();
		}
	}
}

/*
 * @func:			I2C_init
 *
 * @brief:			This function configures the given I2C peripheral
 *
 * @param[in]:		address of I2C Handle for the peripheral
 *
 * @return:			none
 *
 * @note:			for system clock source, only HSI and HSE are considered, PLL and PLLR are not considered
 * 					only 7 bit addresses are considered
 */
void I2C_init(I2C_Handle_t *p_I2C_Handle)
{
	uint32_t temp;
	uint32_t PCLK1_freq_hz = RCC_get_pclk1();

	//enable peripheral clock
	I2C_clock_control(p_I2C_Handle->p_I2Cx, ENABLE);


	//configure clock frequency
	uint32_t PCLK1_freq_Mhz = PCLK1_freq_hz / 1000000;	//FREQ only support Mhz
	temp = p_I2C_Handle->p_I2Cx->CR2;
	temp &= ~(0b11111 << I2C_CR2_FREQ);
	temp |= (PCLK1_freq_Mhz << I2C_CR2_FREQ);
	p_I2C_Handle->p_I2Cx->CR2 = temp;

	//configure device address mode
	temp = p_I2C_Handle->p_I2Cx->OAR1;
	temp &= ~(1 << I2C_OAR_ADDMODE);		//7 bit address

	//configure device address
	temp &= ~(0x7F << 1);
	temp |= ( (p_I2C_Handle->I2Cx_config.I2C_device_addr & 0x7F) << 1);

	temp |= (1 << 14);	//keep 14th bit of OAR 1, as required by data sheet
	p_I2C_Handle->p_I2Cx->OAR1 = temp;

	//configure clock control register
	uint32_t CCR;
	temp = p_I2C_Handle->p_I2Cx->CCR;
	uint32_t clock_speed = p_I2C_Handle->I2Cx_config.I2C_CLK_speed;
	if( clock_speed <= I2C_CLK_SPEED_SM)
	{
		//standard mode
		temp &= ~(1 << I2C_CCR_FS);		//set mode to SM
		//calculate CCR
		CCR = PCLK1_freq_hz / (2 * clock_speed);
	}
	else
	{
		//fast mode
		temp |= (1 << I2C_CCR_FS);		//set mode to FM
		//set FR duty cycle
		uint8_t duty = p_I2C_Handle->I2Cx_config.I2C_FM_duty_cycle;
		if(duty)	//t(low)/t(high) = 16/9
		{
			//set DUTY to 1
			temp |= (duty << I2C_CCR_DUTY);
			//calculate CCR
			CCR = PCLK1_freq_hz / (25 * clock_speed);
		}
		else		//t(low)/t(high) = 2
		{
			//set DUTY to 0
			temp &= ~(1 << I2C_CCR_DUTY);
			//calculate CCR
			 CCR = PCLK1_freq_hz / (3 * clock_speed);

		}
	}
	temp &= ~(0xFFF << I2C_CCR_CCR);
	temp |= ((CCR & 0xFFF) << I2C_CCR_CCR);
	p_I2C_Handle->p_I2Cx->CCR = temp;

	//configure T(rise)
	temp = p_I2C_Handle->p_I2Cx->TRISE;
	temp &= ~(0x3F);
	if(clock_speed <= I2C_CLK_SPEED_SM)
	{
		//standard mode, T(rise) max = 1000ns
		temp |= (PCLK1_freq_hz / 1000000) + 1;		//add 1, as specified in data sheet
	}
	else
	{
		//fast mode, T(rise) max = 300ns
		temp |= ((PCLK1_freq_hz * 300) / 1000000000) + 1;
	}
	p_I2C_Handle->p_I2Cx->TRISE = (temp & 0x3F);
}


/*
 * @func:			I2C_deinit
 *
 * @brief:			This function disables the clock of the given I2C peripheral
 *
 * @param[in]:		address of the I2C peripheral
 *
 * @return:			none
 */
void I2C_deinit(I2C_reg_t *p_I2Cx){
	if(p_I2Cx == I2C1)
	{
		I2C1_REG_RESET();
	}
	else if(p_I2Cx == I2C2)
	{
		I2C2_REG_RESET();
	}
	else if(p_I2Cx == I2C3)
	{
		I2C3_REG_RESET();
	}
}

/*
 * @func:			I2C_controller_send
 *
 * @brief:			This function sends the given data to the target at the given address
 *
 * @param[in]:		address of the I2C peripheral
 * @param[in]:		address of the Tx buffer
 * @param[in]:		how many bytes of data to send
 * @param[in]:		target address
 * @param[in]:		enable or disable repeated start (I2C_RS_enable or I2C_SR_DISABLE)
 *
 * @return:			none
 *
 * @note:			this is an blocking based call
 */
void I2C_controller_send(I2C_Handle_t *p_I2C_Handle, uint8_t *p_Tx_buffer, uint32_t len, uint8_t target_addr, uint8_t RS_enable)
{
	//generate starting condition
	I2C_generate_start(p_I2C_Handle);
	//check SB flag in SR1 to confirm that start condition is generated
	while(I2C_get_flag_status(p_I2C_Handle->p_I2Cx, 1,  I2C_SR1_SB) == 0);
	//send the address with the r/w bit set to write(0) to target
	I2C_execute_addr_phase(p_I2C_Handle, target_addr, WRITE);
	//check ADDR flag in SR1 to confirm address is sent
	while(I2C_get_flag_status(p_I2C_Handle->p_I2Cx, 1, I2C_SR1_ADDR) == 0);
	//clear the ADDR flag to release SCL stretch(pulled to LOW)
	I2C_clear_ADDR_flag(p_I2C_Handle);
	//send data
	while(len > 0)
	{
		while(I2C_get_flag_status(p_I2C_Handle->p_I2Cx, 1, I2C_SR1_TxE) == 0);
		p_I2C_Handle->p_I2Cx->DR = *p_Tx_buffer;
		p_Tx_buffer++;
		len--;
	}
	//wait until TxE=1 (data register is empty) and BTF=1 (byte transfer is finished) to close communication
	while(I2C_get_flag_status(p_I2C_Handle->p_I2Cx, 1, I2C_SR1_TxE) == 0);
	while(I2C_get_flag_status(p_I2C_Handle->p_I2Cx, 1, I2C_SR1_BTF) == 0);
	//generate stop condition if repeated start disabled
	if(RS_enable == I2C_RS_DISABLE)
		I2C_generate_stop(p_I2C_Handle);
}

/*
 * @func:			I2C_controller_receive
 *
 * @brief:			This function receives the given data from the target at the given address
 *
 * @param[in]:		address of the I2C peripheral
 * @param[in]:		address of the Rx buffer
 * @param[in]:		how many bytes of data to receive
 * @param[in]:		target address
 * @param[in]:		enable or disable repeated start (I2C_RS_enable or I2C_SR_DISABLE)
 *
 * @return:			none
 *
 * @note:			this is an blocking based call
 */
void I2C_controller_receive(I2C_Handle_t *p_I2C_Handle, uint8_t *p_Rx_buffer, uint32_t len, uint8_t target_addr, uint8_t RS_enable)
{
	//generate start condition
	I2C_generate_start(p_I2C_Handle);
	//check SB flag in SR1 to confirm that start condition is generated
	while(I2C_get_flag_status(p_I2C_Handle->p_I2Cx, 1,  I2C_SR1_SB) == 0);
	//send the address  to target with the r/w bit set to write(0)
	I2C_execute_addr_phase(p_I2C_Handle, target_addr, READ);
	//check ADDR flag in SR1 to confirm address is sent
	while(I2C_get_flag_status(p_I2C_Handle->p_I2Cx, 1, I2C_SR1_ADDR) == 0);

	//receiving only one byte of data
	/*
	 * note:
	 * 		for len == 1, must disable ACK BEFORE clearing ADDR flag. Clearing ADDR releases SCL, and byte transfer
	 * 		starts immediately. ACK/NACK will be sent on the 9th clock (end of byte). So if ACK bit is not disabled
	 * 		before ADDR clear, hardware will ACK, causing target to send an extra unwanted byte.
	 *
	 *		for len >= 2, can clear ADDR flag as usual and disable ACK and generate STOP when len == 2,
	 *		so the second last byte gets ACK and last byte gets NACK.
	 *
	 */
	if(len == 1)
	{
		//disable ACK
		I2C_manage_acking(p_I2C_Handle, DISABLE);
		//clear the ADDR flag to release SCL stretch(pulled to LOW), so target start transmitting the data
		I2C_clear_ADDR_flag(p_I2C_Handle);
		//wait until RXNE is set to 1
		while(I2C_get_flag_status(p_I2C_Handle->p_I2Cx, 1,  I2C_SR1_RxNE) == 0);
		//generate stop condition
		if(RS_enable == I2C_RS_DISABLE)
			I2C_generate_stop(p_I2C_Handle);
		//read data from DR to Rx buffer
		*p_Rx_buffer = p_I2C_Handle->p_I2Cx->DR;
	}
	else
	{
		//clear ADDR flag so data reception begins
		I2C_clear_ADDR_flag(p_I2C_Handle);
		while(len > 0)
		{
			//wait until RXNE is set to 1, ready to read
			while(I2C_get_flag_status(p_I2C_Handle->p_I2Cx, 1,  I2C_SR1_RxNE) == 0);

			if(len == 2)		//if only 2 bytes are left to receive
			{
				//disable ACK
				I2C_manage_acking(p_I2C_Handle, DISABLE);
				//generate stop condition
				if(RS_enable == I2C_RS_DISABLE)
					I2C_generate_stop(p_I2C_Handle);
			}

			//read data from DR to Rx buffer
			*p_Rx_buffer = p_I2C_Handle->p_I2Cx->DR;
			len--;
			p_Rx_buffer++;

		}
	}
	//re-enalbe ACK if configured
	if(p_I2C_Handle->I2Cx_config.I2C_ACK_control == I2C_ACK_ENABLE)
	{
		I2C_manage_acking(p_I2C_Handle, ENABLE);
	}
}

/*
 * @func:			I2C_target_send
 *
 * @brief:			This function sends data to controller
 *
 * @param[in]:		address of the I2C peripheral of controller
 * @param[in]:		data to send
 *
 * @return:			none
 */
void I2C_target_send(I2C_reg_t *p_I2Cx, uint8_t data)
{
	p_I2Cx->DR = data;
}

/*
 * @func:			I2C_target_receive
 *
 * @brief:			This function reads data sent from controller
 *
 * @param[in]:		address of the I2C peripheral of controller
 *
 * @return:			byte of data received
 */
uint8_t I2C_target_receive(I2C_reg_t *p_I2Cx)
{
	return (uint8_t)p_I2Cx->DR;
}

/*
 * @func:			I2C_Controller_send_IT
 *
 * @brief:			This function start the sending process by setting START condition and enabling interrupts
 *
 * @param[in]:		address of the I2C peripheral
 * @param[in]:		address of the Rx buffer
 * @param[in]:		how many bytes of data to receive
 * @param[in]:		target address
 * @param[in]:		enable or disable repeated start (I2C_RS_enable or I2C_SR_DISABLE)
 *
 * @return:			the state of the I2C peripheral when entering the function, not updated if state is READY entered
 *
 * @note:			this function only initiates the process, the actually reception of data is done in handlers
 */
uint8_t I2C_controller_send_IT(I2C_Handle_t *p_I2C_Handle, uint8_t *p_Tx_buffer, uint32_t len, uint8_t target_addr, uint8_t RS_enable)
{
	uint8_t state = p_I2C_Handle->TxRxstate;

	if((state != I2C_STATE_BUSY_RX) && (state != I2C_STATE_BUSY_TX))
	{
		//save the information to I2C handler
		p_I2C_Handle->p_Tx_buffer = p_Tx_buffer;
		p_I2C_Handle->Tx_len = len;
		p_I2C_Handle->TxRxstate = I2C_STATE_BUSY_TX;
		p_I2C_Handle->repeated_start = RS_enable;
		p_I2C_Handle->target_addr = target_addr;

		//generate start condition
		I2C_generate_start(p_I2C_Handle);

		//enable ITBUFEN(buffer interrupt enable)
		p_I2C_Handle->p_I2Cx->CR2 |= (1 << I2C_CR2_ITBUFEN);
		//enable ITEVTEN(event interrupt enable)
		p_I2C_Handle->p_I2Cx->CR2 |= (1 << I2C_CR2_ITEVTEN);
		//enable ITERREN(error interrupt enable)
		p_I2C_Handle->p_I2Cx->CR2 |= (1 << I2C_CR2_ITERREN);
	}
	return state;

}

/*
 * @func:			I2C_Controller_receive_IT
 *
 * @brief:			This function start the receiving process by setting START condition and enabling interrupts
 *
 * @param[in]:		address of the I2C peripheral
 * @param[in]:		address of the Rx buffer
 * @param[in]:		how many bytes of data to receive
 * @param[in]:		target address
 * @param[in]:		enable or disable repeated start (I2C_RS_enable or I2C_SR_DISABLE)
 *
 * @return:			the state of the I2C peripheral when entering the function, not updated if state is READY entered
 *
 * @note:			this function only initiates the process, the actually reception of data is done in handlers
 */
uint8_t I2C_controller_receive_IT(I2C_Handle_t *p_I2C_Handle, uint8_t *p_Rx_buffer, uint32_t len, uint8_t target_addr, uint8_t RS_enable)
{
	uint8_t state = p_I2C_Handle->TxRxstate;

	if((state != I2C_STATE_BUSY_RX) && (state != I2C_STATE_BUSY_TX))
	{
		//save the information to I2C handler
		p_I2C_Handle->p_Rx_buffer = p_Rx_buffer;
		p_I2C_Handle->Rx_len = len;
		p_I2C_Handle->Rx_size = len;
		p_I2C_Handle->TxRxstate = I2C_STATE_BUSY_RX;
		p_I2C_Handle->repeated_start = RS_enable;
		p_I2C_Handle->target_addr = target_addr;


		//generate start condition
		I2C_generate_start(p_I2C_Handle);

		//enable ITBUFEN(buffer interrupt enable)
		p_I2C_Handle->p_I2Cx->CR2 |= (1 << I2C_CR2_ITBUFEN);
		//enable ITEVTEN(event interrupt enable)
		p_I2C_Handle->p_I2Cx->CR2 |= (1 << I2C_CR2_ITEVTEN);
		//enable ITERREN(error interrupt enable)
		p_I2C_Handle->p_I2Cx->CR2 |= (1 << I2C_CR2_ITERREN);
	}
	return state;
}

/*
 * @func:				I2C_IRQ_config
 *
 * @brief:				This function enable/disable interrupt for the given peripheral
 *
 * @param[in]:			the IRQ number to enable/disable
 * @param[in]:			ENABLE or DISABLE the IRQ
 *
 * @return: 			none
 */
void I2C_IRQ_config(uint8_t IRQ_num, uint8_t enable)
{
	//enable the IRQ
	if(enable == ENABLE)
	{
		if(IRQ_num < 32)
		{
			//enable ISER0
			*NVIC_ISER0 |= (1 << IRQ_num);
		}
		else if(IRQ_num >= 32 && IRQ_num < 64)
		{
			//enable ISER1
			*NVIC_ISER1 |= (1 << (IRQ_num % 32));
		}
		else if(IRQ_num >= 64 && IRQ_num < 96){
			//enable ISER2
			*NVIC_ISER2 |= (1 << (IRQ_num % 64));
		}
		else if (IRQ_num >= 96 && IRQ_num < 128)
		{
			//enable ISER3
			*NVIC_ISER3 |= (1 << (IRQ_num % 96));
		}
	}else{ //disable the IRQ
		if(IRQ_num < 32)
		{
			//enable ICER0
			*NVIC_ICER0 |= (1 << IRQ_num);
		}
		else if(IRQ_num >= 32 && IRQ_num < 64)
		{
			//enable ICER1
			*NVIC_ICER1 |= (1 << (IRQ_num % 32));
		}
		else if(IRQ_num >= 64 && IRQ_num < 96){
			//enable ICER2
			*NVIC_ICER2 |= (1 << (IRQ_num % 64));
		}
		else if (IRQ_num >= 96 && IRQ_num < 128)
		{
			//enable ICER3
			*NVIC_ICER3 |= (1 << (IRQ_num % 96));
		}
	}

}

/*
 * @func:			I2C_set_priority
 *
 * @brief:			This function enable/disable the GPIO pin as given
 *
 * @param[in]:			IRQ number of the peripheral to set priority
 * @param[in]:			priority value to set the IRQ to
 *
 * @return: 		none
 */
void I2C_set_priority(uint8_t IRQ_num, uint8_t IRQ_priority)
{
	//set priority
	uint8_t iprx = IRQ_num / 4;						//which IRQ register, each IPR register only contain 4 interrupts (1 byte apart)
	uint8_t iprx_section = IRQ_num % 4;				//which interrupt(byte) within the IPR register
	uint8_t shift_amount = (8 * iprx_section) + 4; 	//add 4 because the upper 4 bits are the preemptive priority and the lower 4 are the subpriority

	*(NVIC_IPR_BASEADDR + iprx) |= (IRQ_priority << shift_amount); //NVIC_IPR_BASEADDR is uin32_t pointer so adding the iprx will be 4 bytes apart
}

void I2C_EV_IRQ_handling(I2C_Handle_t *p_I2C_Handle)
{
	uint8_t temp1 = ( (p_I2C_Handle->p_I2Cx->CR2 >> I2C_CR2_ITBUFEN) & 1 );
	uint8_t temp2 = ( (p_I2C_Handle->p_I2Cx->CR2 >> I2C_CR2_ITEVTEN) & 1 );

	//handle interrupt generated by SB
	//note: this will only execute for controller mode. In target mode, SB is always 0
	uint8_t temp3 = ( (p_I2C_Handle->p_I2Cx->SR1 >> I2C_SR1_SB) & 1 );
	if(temp2 && temp3)
	{
		//when SB is set, it means address phase should be sent
		if(p_I2C_Handle->TxRxstate == I2C_STATE_BUSY_RX)
		{
			I2C_execute_addr_phase(p_I2C_Handle, p_I2C_Handle->target_addr, READ);
		}
		else if (p_I2C_Handle->TxRxstate == I2C_STATE_BUSY_TX)
		{
			I2C_execute_addr_phase(p_I2C_Handle, p_I2C_Handle->target_addr, WRITE);
		}
	}

	//handle interrupt generated by ADDR
	temp3 = ( (p_I2C_Handle->p_I2Cx->SR1 >> I2C_SR1_ADDR) & 1 );
	if(temp2 && temp3)
	{
		//disable ACK first before clearing ADDR for controller receive
		if( (p_I2C_Handle->p_I2Cx->SR2 >> I2C_SR2_MSL) & 1)	//device in controller mode
		{
			if(p_I2C_Handle->TxRxstate == I2C_STATE_BUSY_RX)	//device is receiving
			{
				if(p_I2C_Handle->Rx_size == 1)
				{
					//disable ACK
					p_I2C_Handle->p_I2Cx->CR1 &= ~(1 << I2C_CR1_ACK);

					//clear the ADDR flag
					I2C_clear_ADDR_flag(p_I2C_Handle);
				}
			}
			else
			{
				//device is sending data, so address phase was successful sent and received,
				//now clear ADDR flag so transmission continues
				I2C_clear_ADDR_flag(p_I2C_Handle);
			}
		}
		else
		{
			//device is target mode
			I2C_clear_ADDR_flag(p_I2C_Handle);
		}

	}

	//handle interrupt generated by BTF
	temp3 = ( (p_I2C_Handle->p_I2Cx->SR1 >> I2C_SR1_BTF) & 1 );
	if(temp2 && temp3)
	{
		if(p_I2C_Handle->TxRxstate == I2C_STATE_BUSY_TX)
		{
			if((p_I2C_Handle->p_I2Cx->SR1 >> I2C_SR1_TxE) & 1)
			{
				//both BTF = 1 and TXE = 1

				//generate stop condition if all data was sent and repeated start disabled
				if( (p_I2C_Handle->Tx_len == 0) && (p_I2C_Handle->repeated_start == I2C_RS_DISABLE)	)
				{
					I2C_generate_stop(p_I2C_Handle);
				}

				//reset Tx related elements of handle structure
				I2C_close_send(p_I2C_Handle);

				//notify user application that transmission completed
				I2C_event_callback(p_I2C_Handle, I2C_EV_TX_CMPLT);
			}
		}
	}

	//handle interrupt generated by STOPF
	//note: this will only execute for target mode, since controller mode will not receive a stop flag, only target
	//receives notice to stop when controller is receiving
	temp3 = ( (p_I2C_Handle->p_I2Cx->SR1 >> I2C_SR1_STOPF) & 1 );
	if(temp2 && temp3)
	{
		//clear STOPF, read SR1(already done above) then write to CR1
		p_I2C_Handle->p_I2Cx->CR1 |= 0;	//a dummy write

		//notify user application that stoop is detected
		I2C_event_callback(p_I2C_Handle, I2C_EV_STOP);
	}


	//handle interrupt generated by TXE
	temp3 = ( (p_I2C_Handle->p_I2Cx->SR1 >> I2C_SR1_TxE) & 1 );
	if(temp1 && temp2 && temp3)
	{
		//TXE flag is set, do data transmission
		if( (p_I2C_Handle->p_I2Cx->SR2 >> I2C_SR2_MSL) & 1 ) //send data only if device is controller
		{
			if(p_I2C_Handle->TxRxstate == I2C_STATE_BUSY_TX) //send only if the peripheral is transmitting
			{
				//send the data
				I2C_controller_TXE_handler(p_I2C_Handle);
			}
		}
		else  //device is target mode
		{
			if( (p_I2C_Handle->p_I2Cx->SR2 >> I2C_SR2_TRA) & 1)	//ensure device is in transmitter mode
			{
				//notify user applicator
				I2C_event_callback(p_I2C_Handle, I2C_EV_DATA_REQ);
			}
		}
	}

	//handle interrupt generated by RXNE
	temp3 = ( (p_I2C_Handle->p_I2Cx->SR1 >> I2C_SR1_RxNE) & 1 );
	if(temp1 && temp2 && temp3)
	{
		if( (p_I2C_Handle->p_I2Cx->SR2 >> I2C_SR2_MSL) & 1)		//device in controller mode
		{
			//RXNE flag is set, do data reception
			if(p_I2C_Handle->TxRxstate == I2C_STATE_BUSY_RX)	//device is receiving data
			{
				I2C_controller_RXNE_handler(p_I2C_Handle);
			}
		}
		else  //device is target mode
		{
			if( ((p_I2C_Handle->p_I2Cx->SR2 >> I2C_SR2_TRA) & 1) == 0)	//ensure device is in receiver mode
			{
				//notify user applicator
				I2C_event_callback(p_I2C_Handle, I2C_EV_DATA_REC);
			}
		}
	}
}


void I2C_ER_IRQ_handling(I2C_Handle_t *p_I2C_Handle)
{
	uint8_t temp1 = ( (p_I2C_Handle->p_I2Cx->CR2 >> I2C_CR2_ITERREN) & 1 );

	//handle error generated by BERR
	uint8_t temp2 = ( (p_I2C_Handle->p_I2Cx->SR1 >> I2C_SR1_BERR) & 1 );
	if(temp1 & temp2)
	{
		//clear BERR
		p_I2C_Handle->p_I2Cx->SR1 &= ~(1 << I2C_SR1_BERR);
		//notify user application
		I2C_event_callback(p_I2C_Handle, I2C_ER_BERR);
	}

	//handle error generated by ARLO
	temp2 = ( (p_I2C_Handle->p_I2Cx->SR1 >> I2C_SR1_ARLO) & 1 );
	if(temp1 & temp2)
	{
		//clear ARLO
		p_I2C_Handle->p_I2Cx->SR1 &= ~(1 << I2C_SR1_ARLO);
		//notify user application
		I2C_event_callback(p_I2C_Handle, I2C_ER_ARLO);
	}

	//handle error generated by AF
	temp2 = ( (p_I2C_Handle->p_I2Cx->SR1 >> I2C_SR1_AF) & 1 );
	if(temp1 & temp2)
	{
		//clear BERR
		p_I2C_Handle->p_I2Cx->SR1 &= ~(1 << I2C_SR1_AF);
		//notify user application
		I2C_event_callback(p_I2C_Handle, I2C_ER_AF);
	}

	//handle error generated by OVR
	temp2 = ( (p_I2C_Handle->p_I2Cx->SR1 >> I2C_SR1_OVR) & 1 );
	if(temp1 & temp2)
	{
		//clear BERR
		p_I2C_Handle->p_I2Cx->SR1 &= ~(1 << I2C_SR1_OVR);
		//notify user application
		I2C_event_callback(p_I2C_Handle, I2C_ER_OVR);
	}

	//handle error generated by PECERR
	temp2 = ( (p_I2C_Handle->p_I2Cx->SR1 >> I2C_SR1_PECERR) & 1 );
	if(temp1 & temp2)
	{
		//clear BERR
		p_I2C_Handle->p_I2Cx->SR1 &= ~(1 << I2C_SR1_PECERR);
		//notify user application
		I2C_event_callback(p_I2C_Handle, I2C_ER_PECERR);
	}

	//handle error generated by TIMEOUT
	temp2 = ( (p_I2C_Handle->p_I2Cx->SR1 >> I2C_SR1_TIMEOUT) & 1 );
	if(temp1 & temp2)
	{
		//clear BERR
		p_I2C_Handle->p_I2Cx->SR1 &= ~(1 << I2C_SR1_TIMEOUT);
		//notify user application
		I2C_event_callback(p_I2C_Handle, I2C_ER_TIMEOUT);
	}

	//handle error generated by SMBALERT
	temp2 = ( (p_I2C_Handle->p_I2Cx->SR1 >> I2C_SR1_SMBALERT) & 1 );
	if(temp1 & temp2)
	{
		//clear BERR
		p_I2C_Handle->p_I2Cx->SR1 &= ~(1 << I2C_SR1_SMBALERT);
		//notify user application
		I2C_event_callback(p_I2C_Handle, I2C_ER_SMBALERT);
	}

}

static void I2C_controller_TXE_handler(I2C_Handle_t *p_I2C_Handle)
{
	if(p_I2C_Handle->Tx_len > 0) //send only if there are more bytes to send
	{
		//load data into DR
		p_I2C_Handle->p_I2Cx->DR = *p_I2C_Handle->p_Tx_buffer;
		p_I2C_Handle->p_Tx_buffer++;
		p_I2C_Handle->Tx_len--;
	}
}
static void I2C_controller_RXNE_handler(I2C_Handle_t *p_I2C_Handle)
{
	if( p_I2C_Handle->Rx_size == 1 )
	{
		*p_I2C_Handle->p_Rx_buffer = p_I2C_Handle->p_I2Cx->DR;
		p_I2C_Handle->Rx_len--;
	}

	if ( p_I2C_Handle->Rx_size > 1 )
	{

		if( p_I2C_Handle->Rx_len == 2)
		{
			//clear ACK
			I2C_manage_acking(p_I2C_Handle, DISABLE);
		}

		*p_I2C_Handle->p_Rx_buffer = p_I2C_Handle->p_I2Cx->DR;
		p_I2C_Handle->Rx_len--;
		p_I2C_Handle->p_Rx_buffer++;
	}

	if(p_I2C_Handle->Rx_len == 0)
	{
		//generate stop condition
		if(p_I2C_Handle->repeated_start == I2C_RS_DISABLE)
			I2C_generate_stop(p_I2C_Handle);
		//close the I2C rx
		I2C_close_receive(p_I2C_Handle);
		//notify user application
		I2C_event_callback(p_I2C_Handle, I2C_EV_RX_CMPLT);

	}
}

void I2C_close_send(I2C_Handle_t *p_I2C_Handle)
{
	//disable ITBUFEN(buffer interrupt enable)
	p_I2C_Handle->p_I2Cx->CR2 &= ~(1 << I2C_CR2_ITBUFEN);
	//disable ITEVTEN(event interrupt enable)
	p_I2C_Handle->p_I2Cx->CR2 &= ~(1 << I2C_CR2_ITEVTEN);

	//clear I2C handler variables
	p_I2C_Handle->TxRxstate = I2C_STATE_READY;
	p_I2C_Handle->p_Tx_buffer = NULL;
	p_I2C_Handle->Tx_len = 0;

	//enable ACK if confugured
	if(p_I2C_Handle->I2Cx_config.I2C_ACK_control == I2C_ACK_ENABLE)
		I2C_manage_acking(p_I2C_Handle, ENABLE);
}

void I2C_close_receive(I2C_Handle_t *p_I2C_Handle)
{
	//disable ITBUFEN(buffer interrupt enable)
	p_I2C_Handle->p_I2Cx->CR2 &= ~(1 << I2C_CR2_ITBUFEN);
	//disable ITEVTEN(event interrupt enable)
	p_I2C_Handle->p_I2Cx->CR2 &= ~(1 << I2C_CR2_ITEVTEN);

	//clear I2C handler variables
	p_I2C_Handle->TxRxstate = I2C_STATE_READY;
	p_I2C_Handle->p_Rx_buffer = NULL;
	p_I2C_Handle->Rx_len = 0;
	p_I2C_Handle->Rx_size = 0;

	//enable ACK if confugured
	if(p_I2C_Handle->I2Cx_config.I2C_ACK_control == I2C_ACK_ENABLE)
		I2C_manage_acking(p_I2C_Handle, ENABLE);
}

/*
 * @func:		I2C_periph_control
 *
 * @brief:		This function enables the given I2C peripheral and the ACK bit as configured
 *
 * @param[in]:	address of the I2C peripheral
 * @param[in]:	ENABLE or DISABLE
 *
 * @return:		none
 *
 * @note:		should be called after I2C_init (after configuration is done)
 */
void I2C_periph_control(I2C_Handle_t *p_I2C_Handle, uint8_t enable)
{
	if(enable == ENABLE)
	{
		p_I2C_Handle->p_I2Cx->CR1 |= (1 << I2C_CR1_PE);
	}else
	{
		p_I2C_Handle->p_I2Cx->CR1 &= ~(1 << I2C_CR1_PE);
	}
	//enable or disable ACK bit accordingly as it should only be enable after PE is set
	if(p_I2C_Handle->I2Cx_config.I2C_ACK_control == I2C_ACK_ENABLE )
	{
		I2C_manage_acking(p_I2C_Handle, ENABLE);
	}
	else
	{
		I2C_manage_acking(p_I2C_Handle, DISABLE);
	}
}

/*
 * @func:			get_flag_status
 *
 * @brief:			This function returns the status of the given flag bit of the I2C status register(SR)
 *
 * @param[in]:		base address of the I2C device
 * @param[in]:		which SR register to read (1 or 2)
 * @param[in]:		the flag bit of the SR register to get status from
 *
 * @return:			the status of the given flag bit
 */
uint8_t I2C_get_flag_status(I2C_reg_t *p_I2Cx, uint8_t SR, uint8_t flag_bit)
{
	if(SR == 1)
	{
		return ( (p_I2Cx->SR1 >> flag_bit) & 1 );
	}
	else
	{
		return ( (p_I2Cx->SR2 >> flag_bit) & 1 );
	}
}

/*
 * @func:			I2C_manage_acking
 *
 * @brief:			This function disable or enable the ACK bit of I2C_CR1 register
 *
 * @param[in]:		base address of the I2C device
 * @param[in]:		ENABLE or DISABLE
 *
 * @return:			the status of the given flag bit
 */
void I2C_manage_acking(I2C_Handle_t *p_I2C_Handle, uint8_t enable)
{
	if(enable == ENABLE)
	{
		p_I2C_Handle->p_I2Cx->CR1 |= (1 << I2C_CR1_ACK);
	}
	else if (enable == DISABLE)
	{
		p_I2C_Handle->p_I2Cx->CR1 &= ~(1 << I2C_CR1_ACK);
	}
}
/*
 * private helper functions
 */

static void I2C_generate_start(I2C_Handle_t *p_I2C_Handle)
{
	p_I2C_Handle->p_I2Cx->CR1 |= (1 << I2C_CR1_START);
}



static void I2C_execute_addr_phase(I2C_Handle_t *p_I2C_Handle, uint8_t target_addr, uint8_t read_or_write)
{
	target_addr = target_addr << 1;
	if(read_or_write == WRITE)
	{
		target_addr &= ~(1);		//r/w bit = 0
	}
	else if(read_or_write == READ)
	{
		target_addr |= 1;		//r/w bit = 1
	}
	p_I2C_Handle->p_I2Cx->DR = target_addr;
}

static void I2C_clear_ADDR_flag(I2C_Handle_t *p_I2C_Handle)
{
	//clear the ADDR flag by reading SR1 and SR2
	uint32_t dummy = p_I2C_Handle->p_I2Cx->SR1;
	dummy = p_I2C_Handle->p_I2Cx->SR2;
	(void)dummy;
}


void I2C_generate_stop(I2C_Handle_t *p_I2C_Handle)
{
	p_I2C_Handle->p_I2Cx->CR1 |= (1 << I2C_CR1_STOP);
}
