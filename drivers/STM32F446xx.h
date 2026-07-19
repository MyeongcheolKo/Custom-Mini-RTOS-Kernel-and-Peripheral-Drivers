/*
 * STM32F446xx.h
 *
 *  Created on: Dec 26, 2025
 *      Author: krisko
 */

#include<stdint.h>



#ifndef DRIVERS_STM32F446XX_H_
#define DRIVERS_STM32F446XX_H_

//NVIC register addresses
#define NVIC_ISER0 		(volatile uint32_t*)0xE000E100
#define NVIC_ISER1 		(volatile uint32_t*)0xE000E104
#define NVIC_ISER2 		(volatile uint32_t*)0xE000E108
#define NVIC_ISER3 		(volatile uint32_t*)0xE000E10C

#define NVIC_ICER0 		(volatile uint32_t*)0XE000E180
#define NVIC_ICER1 		(volatile uint32_t*)0XE000E184
#define NVIC_ICER2 		(volatile uint32_t*)0XE000E188
#define NVIC_ICER3 		(volatile uint32_t*)0XE000E18C

#define NVIC_IPR_BASEADDR 		(volatile uint32_t*)0xE000E400



//base address of FLASH and SRAM
#define FLASH_BASEADDR 	0x08000000U
#define ROM_BASEADDR 	0x1FFF0000U
#define SRAM1_BASEADDR 	0x20000000U
#define SRAM2_BASEADDR 	0x2001C000U
#define SRAM 			SRAM1_BASEADDR


//base address of AHB and APB bus
#define APB1_BASEADDR 	0x40000000U
#define APB2_BASEADDR 	0x40010000U
#define AHB1_BASEADDR 	0x40020000U
#define AHB2_BASEADDR 	0x50000000U
#define AHB3_BASEADDR 	0x60000000U
#define PERIPH_BASEADDR APB1_BASEADDR


//base address of peripherals on AHB1
#define GPIOA_BASEADDR 	(AHB1_BASEADDR + 0x0000)
#define GPIOB_BASEADDR	(AHB1_BASEADDR + 0x0400)
#define GPIOC_BASEADDR 	(AHB1_BASEADDR + 0x0800)
#define GPIOD_BASEADDR 	(AHB1_BASEADDR + 0x0C00)
#define GPIOE_BASEADDR 	(AHB1_BASEADDR + 0x1000)
#define GPIOF_BASEADDR 	(AHB1_BASEADDR + 0x4000)
#define GPIOG_BASEADDR 	(AHB1_BASEADDR + 0x8000)
#define GPIOH_BASEADDR 	(AHB1_BASEADDR + 0xC000)

#define RCC_BASEADDR 	(AHB1_BASEADDR + 0x3800)

//base address of peripherals on APB1
#define SPI2_BASEADDR	(APB1_BASEADDR + 0x3800)
#define SPI3_BASEADDR 	(APB1_BASEADDR + 0x3C00)

#define USART2_BASEADDR (APB1_BASEADDR + 0x4400)
#define USART3_BASEADDR (APB1_BASEADDR + 0x4800)

#define UART4_BASEADDR 	(APB1_BASEADDR + 0x4C00)
#define UART5_BASEADDR 	(APB1_BASEADDR + 0x5000)

#define I2C1_BASEADDR 	(APB1_BASEADDR + 0x5400)
#define I2C2_BASEADDR 	(APB1_BASEADDR + 0x5800)
#define I2C3_BASEADDR 	(APB1_BASEADDR + 0x5C00)

//base address of peripherals on APB2
#define USART1_BASEADDR	(APB2_BASEADDR + 0x1000)
#define USART6_BASEADDR	(APB2_BASEADDR + 0x1400)

#define SPI1_BASEADDR 	(APB2_BASEADDR + 0x3000)
#define SPI4_BASEADDR 	(APB2_BASEADDR + 0x3400)

#define SYSCFG_BASEADDR (APB2_BASEADDR + 0x3800)

#define EXTI_BASEADDR	(APB2_BASEADDR + 0x3C00)


//peripheral definition structures

/*
 * use struct b/c it is very tedious to write out all the address macros for each register
 * by using uint32_t as the variables of the struct, each variable in the struct is 4 bytes or 0x0004 apart, which
 * matches the offsets of the registers so we can access the registers access via the struct, like GPIOA->MODER = ...
 */

//GPIO register structure
typedef struct
{
	volatile uint32_t MODER; 	//GPIO port mode register
	volatile uint32_t OTYPER; 	//GPIO port output type register
	volatile uint32_t OSPEEDR; 	//GPIO port output speed register
	volatile uint32_t PUPDR; 	//GPIO port pull-up/pull-down register
	volatile uint32_t IDR; 		//GPIO port input data register
	volatile uint32_t ODR; 		//GPIO port output data register
	volatile uint32_t BSRR; 	//GPIO port bit set/reset register
	volatile uint32_t LCKR; 	//GPIO port configuration lock register
	volatile uint32_t AFR[2]; 	//GPIO alternate function register, AFR[0] is AFRL and AFR[1] is AFRH
}GPIO_reg_t;

//cast GPIO base addresses to GPIO_reg_t pointers
#define GPIOA 	((GPIO_reg_t*)GPIOA_BASEADDR)
#define GPIOB 	((GPIO_reg_t*)GPIOB_BASEADDR)
#define GPIOC 	((GPIO_reg_t*)GPIOC_BASEADDR)
#define GPIOD 	((GPIO_reg_t*)GPIOD_BASEADDR)
#define GPIOE 	((GPIO_reg_t*)GPIOE_BASEADDR)
#define GPIOF 	((GPIO_reg_t*)GPIOF_BASEADDR)
#define GPIOG 	((GPIO_reg_t*)GPIOG_BASEADDR)
#define GPIOH 	((GPIO_reg_t*)GPIOH_BASEADDR)

//SPI register structure
typedef struct
{
	volatile uint32_t 	CR1;		//SPI control register 1
	volatile uint32_t 	CR2;		//SPI control register 2
	volatile uint32_t 	SR;			//SPI status register
	volatile uint32_t	DR;			//SPI data register
	volatile uint32_t	CRCPR;		//SPI CRC polynomial register
	volatile uint32_t	RXCRCR;		//SPI RX CRC register
	volatile uint32_t	TXCRCR;		//SPI TX CRC register
	volatile uint32_t	I2SCFGR;	//SPI_I2S configuration register
	volatile uint32_t	I2SPR;		//SPI_I2S prescaler register
}SPI_reg_t;

#define SPI1	((SPI_reg_t*)SPI1_BASEADDR)
#define SPI2	((SPI_reg_t*)SPI2_BASEADDR)
#define SPI3	((SPI_reg_t*)SPI3_BASEADDR)
#define SPI4	((SPI_reg_t*)SPI4_BASEADDR)

//I2C register structure
typedef struct
{
	volatile uint32_t CR1;			//I2C control register 1
	volatile uint32_t CR2;			//I2C control register 2
	volatile uint32_t OAR1;			//2C own address register 1
	volatile uint32_t OAR2;			//I2C own address register 2
	volatile uint32_t DR;			//I2C data register
	volatile uint32_t SR1;			//I2C status register 1
	volatile uint32_t SR2;			//I2C status register 2
	volatile uint32_t CCR;			//I2C clock control register
	volatile uint32_t TRISE;		//I2C TRISE register
	volatile uint32_t FLTR;			//I2C FLTR register
}I2C_reg_t;

#define I2C1	((I2C_reg_t*)I2C1_BASEADDR)
#define I2C2	((I2C_reg_t*)I2C2_BASEADDR)
#define I2C3	((I2C_reg_t*)I2C3_BASEADDR)

//USART register structure
typedef struct
{
	volatile uint32_t SR;		//Status register
	volatile uint32_t DR;		//Data register
	volatile uint32_t BRR;		//Baud rate register
	volatile uint32_t CR1;		//Control register 1
	volatile uint32_t CR2;		//Control register 2
	volatile uint32_t CR3;		//Control register 3
	volatile uint32_t GTPR;		//Guard time and prescaler register
}USART_reg_t;

#define USART1	((USART_reg_t*)USART1_BASEADDR)
#define USART2	((USART_reg_t*)USART2_BASEADDR)
#define USART3	((USART_reg_t*)USART3_BASEADDR)
#define UART4	((USART_reg_t*)UART4_BASEADDR)
#define UART5	((USART_reg_t*)UART5_BASEADDR)
#define USART6	((USART_reg_t*)USART6_BASEADDR)


//RCC register structure
typedef struct
{
	volatile uint32_t CR; 			//RCC clock control register
	volatile uint32_t PLLCFGR; 		//RCC PLL configuration register
	volatile uint32_t CFGR;			//RCC clock configuration register
	volatile uint32_t CIR;			//RCC clock interrupt register
	volatile uint32_t AHB1RSTR;		//RCC AHB1 peripheral reset register
	volatile uint32_t AHB2RSTR;		//RCC AHB2 peripheral reset register
	volatile uint32_t AHB3RSTR;		//RCC AHB3 peripheral reset register
	uint32_t RESERVED0;
	volatile uint32_t APB1RSTR; 	//RCC APB1 peripheral reset register
	volatile uint32_t APB2RSTR; 	//RCC APB2 peripheral reset register
	uint32_t RESERVED1[2];
	volatile uint32_t AHB1ENR;		//RCC AHB1 peripheral clock enable register
	volatile uint32_t AHB2ENR;		//RCC AHB2 peripheral clock enable register
	volatile uint32_t AHB3ENR;		//RCC AHB3 peripheral clock enable register
	uint32_t RESERVED2;
	volatile uint32_t APB1ENR;		//RCC APB1 peripheral clock enable register
	volatile uint32_t APB2ENR;		//RCC APB2 peripheral clock enable register
	uint32_t RESERVED3[2];
	volatile uint32_t AHB1LPENR;	//RCC AHB1 peripheral clock enable in low power mode register
	volatile uint32_t AHB2LPENR;	//RCC AHB2 peripheral clock enable in low power mode register
	volatile uint32_t AHB3LPENR;	//RCC AHB3 peripheral clock enable in low power mode register
	uint32_t RESERVED4[2];
	volatile uint32_t APB1LPENR;	//RCC APB1 peripheral clock enable in low power mode register
	volatile uint32_t APB2LPENR;	//RCC APB2 peripheral clock enabled in low power mode register
	uint32_t RESERVED5[2];
	volatile uint32_t BDCR;			//RCC Backup domain control register
	volatile uint32_t CSR;			//RCC clock control and status register
	uint32_t RESERVED6[2];
	volatile uint32_t SSCGR;		//RCC spread spectrum clock generation register
	volatile uint32_t PLLI2SCFGR;	//RCC PLLI2S configuration register
	volatile uint32_t PLLSAICFGR;	//RCC PLL configuration register
	volatile uint32_t DCKCFGR;		//RCC dedicated clock configuration register
	volatile uint32_t CKGATENR;		//RCC clocks gated enable register
	volatile uint32_t DCKCFGR2;		//RCC dedicated clocks configuration register 2
}RCC_reg_t;

#define RCC ((RCC_reg_t*) RCC_BASEADDR)

//EXTI register structure
typedef struct
{
	volatile uint32_t IMR;			//Interrupt mask register
	volatile uint32_t EMR;			//Event mask register
	volatile uint32_t RTSR;			//Rising trigger selection register
	volatile uint32_t FTSR;			//Falling trigger selection register
	volatile uint32_t SWIER;		//Software interrupt event register
	volatile uint32_t PR;			//Pending register
}EXTI_reg_t;

#define EXTI ((EXTI_reg_t*) EXTI_BASEADDR)

//SYSCFG register structure
typedef struct
{
	volatile uint32_t MEMRMP;
	volatile uint32_t PMC;
	volatile uint32_t EXTICR[4];
	volatile uint32_t CMPCR;
	volatile uint32_t CFGR;
}SYSCFG_reg_t;

#define SYSCFG ( (SYSCFG_reg_t*) SYSCFG_BASEADDR)
/*
 * clock enable macros for GPIO peripherals
 */
#define GPIOA_PCLK_EN() ( RCC->AHB1ENR |= (1 << 0) )
#define GPIOB_PCLK_EN() ( RCC->AHB1ENR |= (1 << 1) )
#define GPIOC_PCLK_EN() ( RCC->AHB1ENR |= (1 << 2) )
#define GPIOD_PCLK_EN() ( RCC->AHB1ENR |= (1 << 3) )
#define GPIOE_PCLK_EN() ( RCC->AHB1ENR |= (1 << 4) )
#define GPIOF_PCLK_EN() ( RCC->AHB1ENR |= (1 << 5) )
#define GPIOG_PCLK_EN() ( RCC->AHB1ENR |= (1 << 6) )
#define GPIOH_PCLK_EN() ( RCC->AHB1ENR |= (1 << 7) )

/*
 * clock enable macros for I2C peripherals
 */
#define I2C1_PCLK_EN()	( RCC->APB1ENR |= (1 << 21) )
#define I2C2_PCLK_EN()	( RCC->APB1ENR |= (1 << 22) )
#define I2C3_PCLK_EN()	( RCC->APB1ENR |= (1 << 23) )

/*
 * clock enable macros for SPI peripherals
 */
#define SPI1_PCLK_EN()	( RCC->APB2ENR |= (1 << 12) )
#define SPI2_PCLK_EN()	( RCC->APB1ENR |= (1 << 14) )
#define SPI3_PCLK_EN()	( RCC->APB1ENR |= (1 << 15) )
#define SPI4_PCLK_EN()	( RCC->APB2ENR |= (1 << 13) )

/*
 * clock enable macros for USART peripherals
 */
#define USART1_PCLK_EN()	( RCC->APB2ENR |= (1 << 4) )
#define USART2_PCLK_EN()	( RCC->APB1ENR |= (1 << 17) )
#define USART3_PCLK_EN()	( RCC->APB1ENR |= (1 << 18) )
#define UART4_PCLK_EN()		( RCC->APB1ENR |= (1 << 19) )
#define UART5_PCLK_EN()		( RCC->APB1ENR |= (1 << 20) )
#define USART6_PCLK_EN()	( RCC->APB2ENR |= (1 << 5) )

/*
 * clock enable macros for SYSCFG
 */
#define SYSCFG_PCLK_EN()	( RCC->APB2ENR |= (1 << 14) )

/*
 * clock disable macros for GPIO peripherals
 */
#define GPIOA_PCLK_DI() ( RCC->AHB1ENR &= ~(1 << 0) )
#define GPIOB_PCLK_DI() ( RCC->AHB1ENR &= ~(1 << 1) )
#define GPIOC_PCLK_DI() ( RCC->AHB1ENR &= ~(1 << 2) )
#define GPIOD_PCLK_DI() ( RCC->AHB1ENR &= ~(1 << 3) )
#define GPIOE_PCLK_DI() ( RCC->AHB1ENR &= ~(1 << 4) )
#define GPIOF_PCLK_DI() ( RCC->AHB1ENR &= ~(1 << 5) )
#define GPIOG_PCLK_DI() ( RCC->AHB1ENR &= ~(1 << 6) )
#define GPIOH_PCLK_DI() ( RCC->AHB1ENR &= ~(1 << 7) )

/*
 * clock disable macros for I2C peripherals
 */
#define I2C1_PCLK_DI()	( RCC->APB1ENR &= ~(1 << 21) )
#define I2C2_PCLK_DI()	( RCC->APB1ENR &= ~(1 << 22) )
#define I2C3_PCLK_DI()	( RCC->APB1ENR &= ~(1 << 23) )

/*
 * clock disable macros for SPI peripherals
 */
#define SPI1_PCLK_DI()	( RCC->APB2ENR &= ~(1 << 12) )
#define SPI2_PCLK_DI()	( RCC->APB1ENR &= ~(1 << 14) )
#define SPI3_PCLK_DI()	( RCC->APB1ENR &= ~(1 << 15) )
#define SPI4_PCLK_DI()	( RCC->APB2ENR &= ~(1 << 13) )

/*
 * clock disable macros for USART peripherals
 */
#define USART1_PCLK_DI()	( RCC->APB2ENR &= ~(1 << 4) )
#define USART2_PCLK_DI()	( RCC->APB1ENR &= ~(1 << 17) )
#define USART3_PCLK_DI()	( RCC->APB1ENR &= ~(1 << 18) )
#define UART4_PCLK_DI()		( RCC->APB1ENR &= ~(1 << 19) )
#define UART5_PCLK_DI()		( RCC->APB1ENR &= ~(1 << 20) )
#define USART6_PCLK_DI()	( RCC->APB2ENR &= ~(1 << 5) )

/*
 * macros to reset GPIO peripherals registers
 */
//note: have to set the reset registers back to 0 after setting it to 1 so it is released for later use, otherwise it will be held at reset
#define GPIOA_REG_RESET()		do{ RCC->AHB1RSTR |= (1 << 0); RCC->AHB1RSTR &= ~(1 << 0);}while(0)
#define GPIOB_REG_RESET()		do{ RCC->AHB1RSTR |= (1 << 1); RCC->AHB1RSTR &= ~(1 << 1);}while(0)
#define GPIOC_REG_RESET()		do{ RCC->AHB1RSTR |= (1 << 2); RCC->AHB1RSTR &= ~(1 << 2);}while(0)
#define GPIOD_REG_RESET()		do{ RCC->AHB1RSTR |= (1 << 3); RCC->AHB1RSTR &= ~(1 << 3);}while(0)
#define GPIOE_REG_RESET()		do{ RCC->AHB1RSTR |= (1 << 4); RCC->AHB1RSTR &= ~(1 << 4);}while(0)
#define GPIOF_REG_RESET()		do{ RCC->AHB1RSTR |= (1 << 5); RCC->AHB1RSTR &= ~(1 << 5);}while(0)
#define GPIOG_REG_RESET()		do{ RCC->AHB1RSTR |= (1 << 6); RCC->AHB1RSTR &= ~(1 << 6);}while(0)
#define GPIOH_REG_RESET()		do{ RCC->AHB1RSTR |= (1 << 7); RCC->AHB1RSTR &= ~(1 << 7);}while(0)

/*
 * macros to reset SPI peripheral registers
 */
#define SPI1_REG_RESET()		do{ RCC->APB2RSTR |= (1 << 12); RCC->APB2RSTR &= ~(1 << 12);}while(0)
#define SPI2_REG_RESET()		do{ RCC->APB1RSTR |= (1 << 14); RCC->APB1RSTR &= ~(1 << 14);}while(0)
#define SPI3_REG_RESET()		do{ RCC->APB1RSTR |= (1 << 15); RCC->APB1RSTR &= ~(1 << 15);}while(0)
#define SPI4_REG_RESET()		do{ RCC->APB2RSTR |= (1 << 13); RCC->APB2RSTR &= ~(1 << 13);}while(0)

/*
 * macros to reset I2C peripheral registers
 */
#define I2C1_REG_RESET()		do{ RCC->APB1RSTR |= (1 << 21); RCC->APB1RSTR &= ~(1 << 21);}while(0)
#define I2C2_REG_RESET()		do{ RCC->APB1RSTR |= (1 << 22); RCC->APB1RSTR &= ~(1 << 22);}while(0)
#define I2C3_REG_RESET()		do{ RCC->APB1RSTR |= (1 << 23); RCC->APB1RSTR &= ~(1 << 23);}while(0)

/*
 * macros to reset USART peripheral registers
 */
#define USART1_REG_RESET()		do{ RCC->APB2RSTR |= (1 << 4); RCC->APB2RSTR &= ~(1 << 4);}while(0)
#define USART2_REG_RESET()		do{ RCC->APB1RSTR |= (1 << 17); RCC->APB1RSTR &= ~(1 << 17);}while(0)
#define USART3_REG_RESET()		do{ RCC->APB1RSTR |= (1 << 18); RCC->APB1RSTR &= ~(1 << 18);}while(0)
#define UART4_REG_RESET()		do{ RCC->APB1RSTR |= (1 << 19); RCC->APB1RSTR &= ~(1 << 19);}while(0)
#define UART5_REG_RESET()		do{ RCC->APB1RSTR |= (1 << 20); RCC->APB1RSTR &= ~(1 << 20);}while(0)
#define USART6_REG_RESET()		do{ RCC->APB2RSTR |= (1 << 5); RCC->APB2RSTR &= ~(1 << 5);}while(0)

/*
 * convert GPIO port to port code
 */
#define GPIO_BASEADDR_TO_CODE(x)	((x == GPIOA) ? 0 :\
									(x == GPIOB) ? 1 :\
									(x == GPIOC) ? 2 :\
									(x == GPIOD) ? 3 :\
									(x == GPIOE) ? 4 :\
									(x == GPIOF) ? 5 :\
									(x == GPIOG) ? 6 :\
									(x == GPIOH) ? 7 :0 )
/*
 * IRQ numbers for GPIO, SPI, I2C and USART interrupts
 */
#define IRQ_NO_EXTI0		6
#define IRQ_NO_EXTI1    	7
#define IRQ_NO_EXTI2    	8
#define IRQ_NO_EXTI3    	9
#define IRQ_NO_EXTI4    	10
#define IRQ_NO_EXTI9_5  	23
#define IRQ_NO_EXTI15_10    40

#define IRQ_NO_SPI1			35
#define IRQ_NO_SPI2			36
#define IRQ_NO_SPI3			51
#define IRQ_NO_SPI4			84

#define IRQ_NO_I2C1_EV		31
#define IRQ_NO_I2C1_ER		32
#define IRQ_NO_I2C2_EV		33
#define IRQ_NO_I2C2_ER		34
#define IRQ_NO_I2C3_EV		72
#define IRQ_NO_I2C3_ER		73


/*
 * bit position macros for SPI registers
 */

//SPI_CR1
#define SPI_CR1_CPHA 		0
#define SPI_CR1_CPOL 		1
#define SPI_CR1_MSTR 		2
#define SPI_CR1_BR 			3
#define SPI_CR1_SPE 		6
#define SPI_CR1_LSBFIRST 	7
#define SPI_CR1_SSI 		8
#define SPI_CR1_SSM			9
#define SPI_CR1_RXONLY		10
#define SPI_CR1_DFF			11
#define SPI_CR1_CRCNEXT		12
#define SPI_CR1_CRCEN		13
#define SPI_CR1_BIDIOE		14
#define SPI_CR1_BIDIMODE	15

//SPICR2
#define SPI_CR2_RXDMAEN		0
#define SPI_CR2_TXDMAEN		1
#define SPI_CR2_SSOE		2
#define SPI_CR2_FRF			4
#define SPI_CR2_ERRIE		5
#define SPI_CR2_RXNEIE		6
#define SPI_CR2_TXEIE		7

//SPI_SR
#define SPI_SR_RXNE			0
#define SPI_SR_TXE			1
#define SPI_SR_CHSIDE		2
#define SPI_SR_UDR			3
#define SPI_SR_CRCERR		4
#define SPI_SR_MODF			5
#define SPI_SR_OVR			6
#define SPI_SR_BSY			7
#define SPI_SR_FRE			8

/*
 * bit position macros for I2C registers
 */

//CR1
#define I2C_CR1_PE			0
#define I2C_CR1_SMBUS		1
#define I2C_CR1_SMBTYPE		3
#define I2C_CR1_ENARP		4
#define I2C_CR1_ENPEC		5
#define I2C_CR1_ENGC		6
#define I2C_CR1_NOSTRETCH	7
#define I2C_CR1_START		8
#define I2C_CR1_STOP		9
#define I2C_CR1_ACK			10
#define I2C_CR1_POS			11
#define I2C_CR1_PEC			12
#define I2C_CR1_ALERT		13
#define I2C_CR1_SWRST		15

//CR2
#define I2C_CR2_FREQ		0
#define I2C_CR2_ITERREN		8
#define I2C_CR2_ITEVTEN		9
#define I2C_CR2_ITBUFEN		10
#define I2C_CR2_DMAEN		11
#define I2C_CR2_LAST		12

//OAR
#define I2C_OAR_ADDMODE		15
//SR1
#define I2C_SR1_SB			0
#define I2C_SR1_ADDR		1
#define I2C_SR1_BTF			2
#define I2C_SR1_ADD10		3
#define I2C_SR1_STOPF		4
#define I2C_SR1_RxNE		6
#define I2C_SR1_TxE			7
#define I2C_SR1_BERR		8
#define I2C_SR1_ARLO		9
#define I2C_SR1_AF			10
#define I2C_SR1_OVR			11
#define I2C_SR1_PECERR		12
#define I2C_SR1_TIMEOUT		14
#define I2C_SR1_SMBALERT	15

//SR2
#define I2C_SR2_MSL			0
#define I2C_SR2_BUSY		1
#define I2C_SR2_TRA			2
#define I2C_SR2_GENCALL		4
#define I2C_SR2_SMBDEFAULT	5
#define I2C_SR2_SMBHOST		6
#define I2C_SR2_DUALF		7
#define I2C_SR2_PEC			8

//CCR
#define I2C_CCR_CCR			0
#define I2C_CCR_DUTY		14
#define I2C_CCR_FS			15

/*
 * bit pposition macros for USART registers
 */

//SR
#define USART_SR_PE			0
#define USART_SR_FE			1
#define USART_SR_NF			2
#define USART_SR_ORE		3
#define USART_SR_IDLE		4
#define USART_SR_RXNE		5
#define USART_SR_TC			6
#define USART_SR_TXE		7
#define USART_SR_LBD		8
#define USART_SR_CTS		9

//BRR
#define USART_BRR_DIV_FRACTION	0
#define USART_BRR_DIV_MANTISSA	1

//CR1
#define USART_CR1_SBK		0
#define USART_CR1_RWU		1
#define USART_CR1_RE		2
#define USART_CR1_TE		3
#define USART_CR1_IDLEIE 	4
#define USART_CR1_RXNEIE	5
#define USART_CR1_TCIE		6
#define USART_CR1_TXEIE		7
#define USART_CR1_PEIE		8
#define USART_CR1_PS		9
#define USART_CR1_PCE		10
#define USART_CR1_WAKE		11
#define USART_CR1_M			12
#define USART_CR1_UE		13
#define USART_CR1_OVER8		15

//CR2
#define USART_CR2_ADD		0
#define USART_CR2_LBDL		5
#define USART_CR2_LBDIE		6
#define USART_CR2_LBCL		8
#define USART_CR2_CPHA		9
#define USART_CR2_CPOL		10
#define USART_CR2_CLKEN		11
#define USART_CR2_STOP		12
#define USART_CR2_LINEN		14

//CR3
#define USART_CR3_EIE		0
#define USART_CR3_IREN		1
#define USART_CR3_IRLP		2
#define USART_CR3_HDSEL		3
#define USART_CR3_NACK		4
#define USART_CR3_SCEN		5
#define USART_CR3_DMAR		6
#define USART_CR3_DMAT		7
#define USART_CR3_RTSE		8
#define USART_CR3_CTSE		9
#define USART_CR3_CTSIE		10
#define USART_CR3_ONEBIT	11



//some generic macros
#define ENABLE 		1
#define DISABLE 	0
#define SET 		ENABLE
#define RESET 		DISABLE
#define NULL		((void*)0)
#define READ 		1
#define WRITE		0

#endif /* DRIVERS_STM32F446XX_H_ */
