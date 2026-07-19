/*
 * SPI_driver.c
 *
 *  Created on: Dec 28, 2025
 *      Author: krisko
 */

#include "SPI_driver.h"

/*
 * helper functions for the driver, should not be called by user applications
 */
void static SPI_TXEIE_Handle(SPI_Handle_t *p_SPI_Handle);
void static SPI_RXNEIE_Handle(SPI_Handle_t *p_SPI_Handle);
void static SPI_OVR_Handle(SPI_Handle_t *p_SPI_Handle);
/*
 * @func:			SPI_clock_control
 *
 * @brief:			This function enable/disable the clock for the given SPI peripheral
 *
 * @param[in]:		address of SPI peripheral
 * @param[in]:		ENABLE or DISABLE
 *
 * @return:			none
 */
void SPI_clock_control(SPI_reg_t *p_SPIx, uint8_t enable)
{
	if(enable == ENABLE)
	{
		if(p_SPIx == SPI1)
		{
			SPI1_PCLK_EN();
		}
		else if(p_SPIx == SPI2)
		{
			SPI2_PCLK_EN();
		}
		else if(p_SPIx == SPI3)
		{
			SPI3_PCLK_EN();
		}
		else if(p_SPIx == SPI4)
		{
			SPI4_PCLK_EN();
		}
	}else
	{
		if(p_SPIx == SPI1)
		{
			SPI1_PCLK_DI();
		}
		else if(p_SPIx == SPI2)
		{
			SPI2_PCLK_DI();
		}
		else if(p_SPIx == SPI3)
		{
			SPI3_PCLK_DI();
		}
		else if(p_SPIx == SPI4)
		{
			SPI4_PCLK_DI();
		}
	}
}


/*
 * @func:		SPI_init
 *
 * @brief:		This function configures the SPI peripheral with the given configuration
 *
 * @param[in]:	address of SPI Handle
 *
 * @return:		none
 *
 * @note: 		this function enables the peripheral clock
 */
void SPI_init(SPI_Handle_t *p_SPI_Handle){

	//enable the peripheral clock
	SPI_clock_control(p_SPI_Handle->p_SPIx, ENABLE);

	uint32_t temp = 0;
	temp = p_SPI_Handle->p_SPIx->CR1;


	//configure device mode
	temp &= ~(1 << SPI_CR1_MSTR);
	temp |= (p_SPI_Handle->SPI_config.SPI_device_mode << SPI_CR1_MSTR);

	uint8_t bus = p_SPI_Handle->SPI_config.SPI_bus_config;
	//configure device bus
	if(bus == SPI_BUS_CONFIG_FD){
		//clear BIDIMODE bit -> full-duplex
		temp &= ~(1 << SPI_CR1_BIDIMODE);
		//clear RXONLY bit to be safe
		temp &= ~(1 << SPI_CR1_RXONLY);
	}else if (bus == SPI_BUS_CONFIG_HD)
	{
		//set BIDIMODE bit -> hald-duplex
		temp |= (1 << SPI_CR1_BIDIMODE);
		//clear RXONLY bit to be safe
		temp &= ~(1 << SPI_CR1_RXONLY);
	}else if(bus == SPI_BUS_CONFIG_S_RXONLY)
	{
		//clear BIDIMODE bit
		temp &= ~(1 << SPI_CR1_BIDIMODE);
		//set RXONLY bit
		temp |= (1 << SPI_CR1_RXONLY);
	}

	//configure clock speed
	temp &= ~(0b111 << SPI_CR1_BR);
	temp |= (p_SPI_Handle->SPI_config.SPI_sclk_speed << SPI_CR1_BR);

	//configure DFF
	temp &= ~(1 << SPI_CR1_DFF);
	temp |= (p_SPI_Handle->SPI_config.SPI_DFF << SPI_CR1_DFF);

	//configure CPOL
	temp &= ~(1 << SPI_CR1_CPOL);
	temp |= (p_SPI_Handle->SPI_config.SPI_CPOL << SPI_CR1_CPOL);

	//configure CPHA
	temp &= ~(1 << SPI_CR1_CPHA);
	temp |= (p_SPI_Handle->SPI_config.SPI_CPHA << SPI_CR1_CPHA);

	//configure SSM
	temp &= ~(1 << SPI_CR1_SSM);
	temp |= (p_SPI_Handle->SPI_config.SPI_SSM << SPI_CR1_SSM);

	p_SPI_Handle->p_SPIx->CR1 = temp;
}

/*
 * @func:		SPI_deinit
 *
 * @brief:		This function disables the clock of the given SPI peripheral
 *
 * @param[in]:		address of the SPI peripheral
 *
 * @return:		none
 */
void SPI_deinit(SPI_reg_t *p_SPIx){
	if(p_SPIx == SPI1)
	{
		SPI1_REG_RESET();
	}
	else if(p_SPIx == SPI2)
	{
		SPI2_REG_RESET();
	}
	else if(p_SPIx == SPI3)
	{
		SPI3_REG_RESET();
	}
	else if(p_SPIx == SPI4)
	{
		SPI4_REG_RESET();
	}
}

/*
 * @func:		SPI_periph_control
 *
 * @brief:		This function enables the given SPI peripheral
 *
 * @param[in]:	address of the SPI peripheral
 *
 * @return:		none
 *
 * @note:		should be called after SPI_init (after configuration is done)
 */
void SPI_periph_control(SPI_reg_t *p_SPIx, uint8_t enable)
{
	if(enable == ENABLE)
	{
		p_SPIx->CR1 |= (1 << SPI_CR1_SPE);
	}else
	{
		p_SPIx->CR1 &= ~(1 << SPI_CR1_SPE);
	}
}

/*
 * @func:			SPI_send
 *
 * @brief:			This function sends data to the given SPI peripheral
 *
 * @param[in]:		address of SPI device
 * @param[in]:		address of the Tx buffer that stores data to send
 * @param[in]:		the length of byte to send
 *
 *
 * @return: 		none
 *
 * @note: 			this is a blocking call
 */
void SPI_send(SPI_reg_t *p_SPIx, uint8_t *p_Tx_buffer, uint32_t len){
	while(len > 0)
	{
		//wait until TXE is set (transmit buffer is empty)
		while(SPI_get_flag_status(p_SPIx, SPI_SR_TXE) == 0);

		//check DFF bit
		if(p_SPIx->CR1 & (1 << SPI_CR1_DFF)) //DFF bit is set, 16-bit
		{
			//write to data register(DR)
			p_SPIx->DR = *( (uint16_t*)p_Tx_buffer );  //cast to uint16_t then dereference to get 16 bits
			len -= 2;
			p_Tx_buffer += 2;
		}else		//DFF is not set, 8-bit
		{
			p_SPIx->DR = *p_Tx_buffer;
			len--;
			p_Tx_buffer++;
		}

	}
}

/*
 @ fcn:			SPI_recieve
 *
 * @brief:		This function reads data received at the given SPI peripheral
 *
 * @param[in]:		address of SPI device
 * @param[in]:		address of the Rx buffer to store received data
 * @param[in]:		the length of byte to recieve
 *
 *
 * @return: 	none
 *
 * @note: 		this is a blocking call
 *
 * @warning：   	For master mode, len should be 1. The master controls SCLK,
 *              so without sending data, no clocks are generated and RXNE
 *              will never be set — causing an infinite loop after the first
 *              iteration. To receive multiple bytes as master, send dummy
 *              bytes and read one byte at a time.
 *
 *              For slave mode, any len value works since the master
 *              provides the clock.
 */
void SPI_recieve(SPI_reg_t *p_SPIx, uint8_t *p_Rx_buffer, uint32_t len){
	while(len > 0)
	{
		//wait until RXNE is set (transmit buffer is empty)
		while(SPI_get_flag_status(p_SPIx, SPI_SR_RXNE) == 0);

		//check DFF bit
		if(p_SPIx->CR1 & (1 << SPI_CR1_DFF)) //DFF bit is set, 16-bit
		{
			//read from data register(DR)
			*((uint16_t*)p_Rx_buffer) = p_SPIx->DR;	//cast p_Rx_buffer to uint16_t so it loads 2 bytes of data from DR
			len -= 2;
			p_Rx_buffer += 2;
		}else		//DFF is not set, 8-bit
		{
			//read from data register(DR)
			*p_Rx_buffer = p_SPIx->DR;
			len--;
			p_Rx_buffer++;
		}

	}
}

/*
 * @func:			SPI_send_IT
 *
 * @brief:			This function initiate SPI data transmission using interrupt mode
 *
 * @param[in]:		address of SPI Handle structure
 * @param[in]:		address of the Tx buffer that stores data to send
 * @param[in]:		the length of byte to send
 *
 * @return: 		whether data is transmission started successfully or not (0 or 1)
 *
 * @note: 			this is a non-blocking call, actual data transmission will be handled by the interrupt handlers
 */
uint8_t SPI_send_IT(SPI_Handle_t *p_SPI_Handle, uint8_t *p_Tx_buffer, uint32_t len)
{
	uint8_t state = p_SPI_Handle->Tx_state;

	//only send the data why peripheral is not in the process of sending data
	if (state != SPI_STATE_BUSY_IN_TX)
	{
		//save the Tx buffer address and len to SPI_Handle
		p_SPI_Handle->p_Tx_buffer = p_Tx_buffer;
		p_SPI_Handle->Tx_len = len;

		//mark the SPI peripheral state as busy in transmission
		p_SPI_Handle->Tx_state = SPI_STATE_BUSY_IN_TX;

		//enable the TXEIE control bit so interrupt will be triggered when TXE flag is set in SR (TXE is empty)
		p_SPI_Handle->p_SPIx->CR2 |= (1 << SPI_CR2_TXEIE);
	}
	return state;
}

/*
 * @func:			SPI_recieve_IT
 *
 * @brief:			This function initiate SPI data receive using interrupt mode
 *
 * @param[in]:		address of SPI Handle structure
 * @param[in]:		address of the Rx buffer that stores data to send
 * @param[in]:		the length of byte to receive
 *
 * @return: 		whether data is receive has started successfully or not (0 or 1)
 *
 * @note: 			this is a non-blocking call, actual data reading will be handled by the interrupt handlers
 */
uint8_t SPI_recieve_IT(SPI_Handle_t *p_SPI_Handle, uint8_t *p_Rx_buffer, uint32_t len)
{
	uint8_t state = p_SPI_Handle->Rx_state;

	//only read the data why peripheral is not in the process of reading data
	if (state != SPI_STATE_BUSY_IN_RX)
	{
		//save the Tx buffer address and len to SPI_Handle
		p_SPI_Handle->p_Rx_buffer = p_Rx_buffer;
		p_SPI_Handle->Rx_len = len;

		//mark the SPI peripheral state as busy in transmission
		p_SPI_Handle->Rx_state = SPI_STATE_BUSY_IN_RX;

		//enable the RXNEIE control bit so interrupt will be triggered when RXNE flag is set in SR (RXNE is not empty)
		p_SPI_Handle->p_SPIx->CR2 |= (1 << SPI_CR2_RXNEIE);
	}
	return state;
}

/*
 * @func:				SPI_IRQ_config
 *
 * @brief:				This function enable/disable interrupt for the given peripheral
 *
 * @param[in]:			the IRQ number to enable/disable
 * @param[in]:			ENABLE or DISABLE the IRQ
 *
 * @return: 			none
 */
void SPI_IRQ_config(uint8_t IRQ_num, uint8_t enable)
{
	//enable the IRQ
	if(enable == ENABLE)
	{
		if(IRQ_num < 32)
		{
			//enable ISER0
			*NVIC_ISER0 |= (1 << IRQ_num);
		}else if(IRQ_num >= 32 && IRQ_num < 64)
		{
			//enable ISER1
			*NVIC_ISER1 |= (1 << (IRQ_num % 32));
		}else if(IRQ_num >= 64 && IRQ_num < 96){
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
		}else if(IRQ_num >= 32 && IRQ_num < 64)
		{
			//enable ICER1
			*NVIC_ICER1 |= (1 << (IRQ_num % 32));
		}else if(IRQ_num >= 64 && IRQ_num < 96){
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
 * @func:			SPI_set_priority
 *
 * @brief:			This function enable/disable the GPIO pin as given
 *
 * @param[in]:			IRQ number of the peripheral to set priority
 * @param[in]:			priority value to set the IRQ to
 *
 * @return: 		none
 */
void SPI_set_priority(uint8_t IRQ_num, uint8_t IRQ_priority)
{
	//set priority
	uint8_t iprx = IRQ_num / 4;						//which IRQ register, each IPR register only contain 4 interrupts (1 byte apart)
	uint8_t iprx_section = IRQ_num % 4;				//which interrupt(byte) within the IPR register
	uint8_t shift_amount = (8 * iprx_section) + 4; 	//add 4 because the upper 4 bits are the preemptive priority and the lower 4 are the subpriority

	*(NVIC_IPR_BASEADDR + iprx) |= (IRQ_priority << shift_amount); //NVIC_IPR_BASEADDR is uin32_t pointer so adding the iprx will be 4 bytes apart
}

/*
 * @func:			SPI_IRQ_handler
 *
 * @brief:			This function identifies the interrupt source and redirects to the appropriate handler
 *
 * @param[in]:		SPI handle structure
 *
 * @return:			none
 *
 * @note:			This driver only handles the following interrupts: TXE, RXNE, OVR
 *              	Other SPI errors are not handled: MODF, CRCERR, FRE
 */
void SPI_IRQ_handler(SPI_Handle_t *p_SPI_Handle){

	uint8_t temp1, temp2;

	//check if interrupt is TXEIE and RXNE is set, ie triggering interrupt when Tx buffer is empty
	temp1 = ( (p_SPI_Handle->p_SPIx->SR  >> SPI_SR_TXE)    & 1);
	temp2 = ( (p_SPI_Handle->p_SPIx->CR2 >> SPI_CR2_TXEIE) & 1);

	if(temp1 & temp2)
	{
		SPI_TXEIE_Handle(p_SPI_Handle);
	}

	//check if interrupt is RXNEIE and RXNE is set, ie triggering interrupt when Rx buffer is not empty
	temp1 = ( (p_SPI_Handle->p_SPIx->SR  >> SPI_SR_RXNE)    & 1);
	temp2 = ( (p_SPI_Handle->p_SPIx->CR2 >> SPI_CR2_RXNEIE) & 1);

	if(temp1 & temp2)
	{
		SPI_RXNEIE_Handle(p_SPI_Handle);
	}

	//check if interrupt is ERRIE and OVR
	temp1 = ( (p_SPI_Handle->p_SPIx->SR  >> SPI_SR_OVR)   & 1);
	temp2 = ( (p_SPI_Handle->p_SPIx->CR2 >> SPI_CR2_ERRIE) & 1);

	if(temp1 & temp2)
	{
		SPI_OVR_Handle(p_SPI_Handle);
	}

}

/*
 * @func:		get_flag_status
 *
 * @brief:		This function returns the status of the given flag bit of the SPI status register(SR)
 *
 * @param[in]:		base address of the SPI device
 * @param[in]:		the flag bit of the SR register to get status from
 *
 * @return:		the status of the given flag bit
 */
uint8_t SPI_get_flag_status(SPI_reg_t *p_SPIx, uint8_t flag_bit)
{
	return ( p_SPIx->SR & (1 << flag_bit) );
}

/*
 * @func:		SPI_SSI_config
 *
 * @brief:		This function enable/disable the SSI bit as given
 *
 * @param[in]:		base address of the SPI device
 * @param[in]:		ENABLE or DISABLE
 *
 * @return:		none
 */
void SPI_SSI_config(SPI_reg_t *p_SPIx, uint8_t enable)
{
	if(enable == ENABLE)
	{
		p_SPIx->CR1 |= (1 << SPI_CR1_SSI);
	}
	else
	{
		p_SPIx->CR1 &= ~(1 << SPI_CR1_SSI);
	}
}

/*
 * @func:			SPI_SSOE_config
 *
 * @brief:			This function enable/disable the SSEO bit as given
 *
 * @param[in]:		address of the SPI peripheral
 * @param[in]:		ENABLE or DISABLE
 *
 * @return:			none
 */
void SPI_SSOE_config(SPI_reg_t *p_SPIx, uint8_t enable)
{
	if(enable == ENABLE)
	{
		p_SPIx->CR2 |= (1 << SPI_CR2_SSOE);
	}
	else
	{
		p_SPIx->CR2 &= ~(1 << SPI_CR2_SSOE);
	}
}


/*
 * @func:			SPI_TXEIE_Handle
 *
 * @brief:			This function sends one byte, informs the user application
 * 					and closes the TXEIE interrupt if all data is sent
 *
 * @param[in]:		address of the SPI Handle structure
 *
 * @return:			none
 *
 * @note:			This function is called by SPI_IRQ_handler
 */
void static SPI_TXEIE_Handle(SPI_Handle_t *p_SPI_Handle)
{
	//check DFF bit
	if(p_SPI_Handle->p_SPIx->CR1 & (1 << SPI_CR1_DFF)) //DFF bit is set, 16-bit
	{
		//write to data register(DR)
		p_SPI_Handle->p_SPIx->DR = *( (uint16_t*)p_SPI_Handle->p_Tx_buffer );  //cast to uint16_t then dereference to write 16 bits
		p_SPI_Handle->Tx_len -= 2;
		p_SPI_Handle->p_Tx_buffer += 2;
	}else		//DFF is not set, 8-bit
	{
		p_SPI_Handle->p_SPIx->DR = *(p_SPI_Handle->p_Tx_buffer);
		p_SPI_Handle->Tx_len--;
		p_SPI_Handle->p_Tx_buffer++;
	}

	//close the SPI transmission if Tx length is 0(no data to transmit)
	//and inform use application that transmission is over
	if(p_SPI_Handle->Tx_len == 0)
	{
		//close transmission
		SPI_close_transmission(p_SPI_Handle);

		//inform user application
		SPI_event_callback(p_SPI_Handle, SPI_EVENT_TX_CMPLT);
	}

}

/*
 * @func:			SPI_RXNEIE_Handle
 *
 * @brief:			This function reads one byte, informs the user application
 * 					and closes the RXNEIE interrupt if all data is read
 *
 * @param[in]:		address of the SPI Handle structure
 *
 * @return:			none
 *
 * @note:			This function is called by SPI_IRQ_handler
 */
void static SPI_RXNEIE_Handle(SPI_Handle_t *p_SPI_Handle)
{
	//check DFF bit
	if(p_SPI_Handle->p_SPIx->CR1 & (1 << SPI_CR1_DFF)) //DFF bit is set, 16-bit
	{
		//read data register(DR)
		*( (uint16_t*)p_SPI_Handle->p_Rx_buffer ) = p_SPI_Handle->p_SPIx->DR;  //cast to uint16_t then dereference to read 16 bits
		p_SPI_Handle->Rx_len -= 2;
		p_SPI_Handle->p_Rx_buffer += 2;
	}else		//DFF is not set, 8-bit
	{
		*(p_SPI_Handle->p_Rx_buffer) = p_SPI_Handle->p_SPIx->DR;
		p_SPI_Handle->Rx_len--;
		p_SPI_Handle->p_Rx_buffer++;
	}

	//close the SPI transmission if Rx length is 0(no data to transmit)
	//and inform use application that transmission is over
	if(p_SPI_Handle->Rx_len == 0)
	{
		//close reception
		SPI_close_reception(p_SPI_Handle);

		//inform user application
		SPI_event_callback(p_SPI_Handle, SPI_EVENT_RX_CMPLT);
	}
}

/*
 * @func:			SPI_OVR_Handle
 *
 * @brief:			This function clears the OVR flag if not in transmission and informs
 * 					user application to clear to clear OVR flag if peripheral is bust
 *
 * @param[in]:		address of the SPI Handle structure
 *
 * @return:			none
 *
 * @note:			This function is called by SPI_IRQ_handler
 */
void static SPI_OVR_Handle(SPI_Handle_t *p_SPI_Handle)
{
	//check if peripheral is busy in transmission b/c overrun error can only occur if a transmission is done when previous
	//Rx buffer for last transmission is not cleared
	if(p_SPI_Handle->Tx_state != SPI_STATE_BUSY_IN_TX)
	{
		//clear OVR flag
		SPI_clear_OVR_flag(p_SPI_Handle);
	}
	//inform user application, should resend the data and clear OVR flag
	SPI_event_callback(p_SPI_Handle, SPI_EVENT_OVR_ERR);
}

/*
 * @func:			SPI_clear_OVR_flag
 *
 * @brief:			This function clears the OVR flag
 *
 * @param[in]:		address of the SPI peripheral
 *
 * @return:			none
 */
void SPI_clear_OVR_flag(SPI_Handle_t *p_SPI_Handle)
{
	uint8_t temp;
	//clear OVR flag
	temp = p_SPI_Handle->p_SPIx->DR;
	temp = p_SPI_Handle->p_SPIx->SR;
	(void)temp;
}

/*
 * @func:			SPI_close_transmission
 *
 * @brief:			This function disables TXEIE and resets transmission related SPI Handler values for the given SPI peripheral
 *
 * @param[in]:		address of the SPI Handle structure
 *
 * @return:			none
 */
void SPI_close_transmission(SPI_Handle_t *p_SPI_Handle)
{
	//reset TXEIE (disable tx buffer empty interrupt)
	p_SPI_Handle->p_SPIx->CR2 &= ~(1 << SPI_CR2_TXEIE);

	//reset SPI Handler values
	p_SPI_Handle->p_Tx_buffer = NULL;
	p_SPI_Handle->Tx_len = 0;
	p_SPI_Handle->Tx_state = SPI_STATE_READY;
}

/*
 * @func:			SPI_close_reception
 *
 * @brief:			This function disables RXNEIE and resets reception related SPI Handler values for the given SPI peripheral
 *
 * @param[in]:		address of the SPI Handle structure
 *
 * @return:			none
 */
void SPI_close_reception(SPI_Handle_t *p_SPI_Handle)
{
	//reset RXNEIE (disable Rx buffer not  empty interrupt)
	p_SPI_Handle->p_SPIx->CR2 &= ~(1 << SPI_CR2_RXNEIE);

	//reset SPI Handler values
	p_SPI_Handle->p_Rx_buffer = NULL;
	p_SPI_Handle->Rx_len = 0;
	p_SPI_Handle->Rx_state = SPI_STATE_READY;
}

/*
 * @func:			SPI_event_callback
 *
 * @brief:			This is a weak implementation of the function and should be overridden by user application
 *
 * @param[in]:		address of the SPI Handle structure
 *
 * @return:			none
 */
__attribute__((weak)) void SPI_event_callback(SPI_Handle_t *p_SPI_Handle, uint8_t event)
{

}
