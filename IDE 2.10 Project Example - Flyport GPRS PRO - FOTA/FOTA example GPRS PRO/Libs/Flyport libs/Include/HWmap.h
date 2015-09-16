/* **************************************************************************																					
 *                                OpenPicus                 www.openpicus.com
 *                                                            italian concept
 * 
 *            openSource wireless Platform for sensors and Internet of Things	
 * **************************************************************************
 *  FileName:        HWmap.h
 *  Dependencies:    Microchip configs files
 *  Module:          FlyPort WI-FI
 *  Compiler:        Microchip C30 v3.12 or higher
 *
  *  Author               Rev.    Date              Comment
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  Gabriele Allegria    1.0     1/20/2011		   First release  (core team)
 *  Andrea Seraghiti     2.2     11/21/2011        Added ethernet support
 *  Simone Marra         2.3     10/22/2012             Added GPRS/3G support
 *  					 2.6	 10/28/2013			Added GPRS/Pro support
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  Software License Agreement
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  This is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License (version 2) as published by 
 *  the Free Software Foundation AND MODIFIED BY OpenPicus team.
 *  
 *  ***NOTE*** The exception to the GPL is included to allow you to distribute
 *  a combined work that includes OpenPicus code without being obliged to 
 *  provide the source code for proprietary components outside of the OpenPicus
 *  code. 
 *  OpenPicus software is distributed in the hope that it will be useful, but 
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details. 
 * 
 * 
 * Warranty
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * THE SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION, ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * WE ARE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF
 * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS
 * BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE
 * THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER
 * SIMILAR COSTS, WHETHER ASSERTED ON THE BASIS OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE), BREACH OF WARRANTY, OR OTHERWISE.
 *
 **************************************************************************/
/*****************************************************************************
 *								--- CLOCK FREQUENCY ---					 	 *
 ****************************************************************************/
#ifndef __MAP_H
#define __MAP_H

#define GetSystemClock()		(32000000ul)      // Hz
#define GetInstructionClock()	(GetSystemClock()/2)
#define GetPeripheralClock()	GetInstructionClock()	

#define FLYPORT_PRO
#define FLYPORTGPRS
	
/*****************************************************************************
 *							--- WIFI MODULE MAPPING ---					 	 *
 ****************************************************************************/
#if defined (FLYPORT)
	#define MRF24WB0M_IN_SPI1
	
	#define WF_CS_TRIS			(TRISGbits.TRISG9)
	#define WF_CS_IO			(LATGbits.LATG9)
	#define WF_SDI_TRIS			(TRISGbits.TRISG7)
	#define WF_SCK_TRIS			(TRISGbits.TRISG6)
	#define WF_SDO_TRIS			(TRISGbits.TRISG8)
	#define WF_RESET_TRIS		(TRISEbits.TRISE7)
	#define WF_RESET_IO			(LATEbits.LATE7)
	#define WF_INT_TRIS	    	(TRISBbits.TRISB2)  // INT1
	#define WF_INT_IO			(PORTBbits.RB2)
	#define WF_HIBERNATE_TRIS	(TRISEbits.TRISE6)
	#define	WF_HIBERNATE_IO		(LATEbits.LATE6)
	#define WF_INT_EDGE		    (INTCON2bits.INT1EP)
	#define WF_INT_IE			(IEC1bits.INT1IE)
	#define WF_INT_IF			(IFS1bits.INT1IF)
	
	#define WF_SSPBUF			(SPI1BUF)
	#define WF_SPISTAT			(SPI1STAT)
	#define WF_SPISTATbits		(SPI1STATbits)
	#define WF_SPICON1			(SPI1CON1)
	#define WF_SPICON1bits		(SPI1CON1bits)
	#define WF_SPICON2			(SPI1CON2)
	#define WF_SPI_IE			(IEC0bits.SPI1IE)
	//#define WF_SPI_IP			(IPC2bits.SPI1IP)
	#define WF_SPI_IF			(IFS0bits.SPI1IF)

#elif defined (FLYPORTETH)

    #define ENC100_INTERFACE_MODE_FLYPORT			5
    #define ENC100_PSP_USE_INDIRECT_RAM_ADDRESSING
    #define ENC100_TRANSLATE_TO_PIN_ADDR(a)		((((a)&0x0100)<<6) | ((a)&0x00FF))

    #define SPIFLASH_CS_TRIS	(TRISBbits.TRISB4)
    #define SPIFLASH_CS_IO		(LATBbits.LATB4)
    #define SPIFLASH_SCK_TRIS	(TRISGbits.TRISG9)
    #define SPIFLASH_SDI_TRIS	(TRISGbits.TRISG7)
    #define SPIFLASH_SDI_IO		(PORTGbits.RG7)
    #define SPIFLASH_SDO_TRIS	(TRISGbits.TRISG8)
    #define SPIFLASH_SPI_IF		(IFS1bits.SPI1IF)
    #define SPIFLASH_SSPBUF		(SPI1BUF)
    #define SPIFLASH_SPICON1	(SPI1CON1)
    #define SPIFLASH_SPICON1bits	(SPI1CON1bits)
    #define SPIFLASH_SPICON2	(SPI1CON2)
    #define SPIFLASH_SPISTAT	(SPI1STAT)
    #define SPIFLASH_SPISTATbits	(SPI1STATbits)
        
#elif defined (FLYPORTGPRS)
    #define TURN_HILO_DTR_ON    0
    #define TURN_HILO_DTR_OFF   1

    // HILO UART1 <--> FLYPORT UART number HILO_UART
	#define HILO_UART               4
	#define HILO_TXD_TRIS           (TRISGbits.TRISG8)
	#define HILO_TXD_IO             (LATGbits.LATG8)
	#define HILO_RXD_TRIS           (TRISGbits.TRISG6)
	#define HILO_RXD_IO             (PORTGbits.RG6)
	#define HILO_DSR_TRIS           (TRISEbits.TRISE2)
	#define HILO_DSR_IO             (PORTEbits.RE2)
	#define HILO_DCD_TRIS           (TRISDbits.TRISD6)
	#define HILO_DCD_IO             (PORTDbits.RD6)
	#define HILO_CTS_TRIS           (TRISGbits.TRISG7)
	#define HILO_CTS_IO             (PORTGbits.RG7)
	#define HILO_DTR_TRIS           (TRISEbits.TRISE0)
	#define HILO_DTR_IO             (LATEbits.LATE0)
	#define HILO_RI_TRIS            (TRISDbits.TRISD3)
	#define HILO_RI_IO              (PORTDbits.RD3)
	#define HILO_RTS_TRIS           (TRISGbits.TRISG9)
	#define HILO_RTS_IO             (LATGbits.LATG9)
/*
    // HILO GPIOs
    #define HILO_GPIO1_TRIS         (TRISDbits.TRISD4)
    #define HILO_GPIO1_PORT         (PORTDbits.RD4)
    #define HILO_GPIO1_LAT          (LATDbits.LATD4)
    #define HILO_GPIO2_TRIS         (TRISBbits.TRISB4)
    #define HILO_GPIO2_PORT         (PORTBbits.RB4)
    #define HILO_GPIO2_LAT          (LATBbits.LATB4)
    #define HILO_GPIO3_TRIS         (TRISBbits.TRISB)
    #define HILO_GPIO3_PORT         (PORTBbits.RB)
    #define HILO_GPIO3_LAT          (LATBbits.LATB)
*/  
	#define HILO_VGPIO_TRIS         (TRISBbits.TRISB12)
    #define HILO_VGPIO_PORT         (PORTBbits.RB12)
    #define HILO_VGPIO_LAT          (LATBbits.LATB12)
    #define HILO_RESET_TRIS         (TRISDbits.TRISD7)
    #define HILO_RESET_IO           (LATDbits.LATD7)
    #define HILO_RESET_ODC          (ODCDbits.ODD7)
	#define HILO_POK_TRIS           (TRISEbits.TRISE1)
	#define HILO_POK_IO             (PORTEbits.RE1)
    #define HILO_POK_ODC            (ODCEbits.ODE1)
    
    
    //	Onboard SPI flash mapping
	#define SPIFLASH_CS_TRIS        (TRISBbits.TRISB11)
	#define SPIFLASH_CS_IO          (LATBbits.LATB11)
	#define SPIFLASH_SCK_TRIS       (TRISBbits.TRISB1)
	//#define SPIFLASH_SCK_TRIS     (TRISFbits.TRISF3)
	#define SPIFLASH_SDI_TRIS       (TRISBbits.TRISB9)
	#define SPIFLASH_SDI_IO         (PORTBbits.RB9)
	#define SPIFLASH_SDO_TRIS       (TRISDbits.TRISD8)
	#define SPIFLASH_SPI_IF         (IFS5bits.SPI1IF)
	#define SPIFLASH_SSPBUF         (SPI1BUF)
	#define SPIFLASH_SPICON1        (SPI1CON1)
	#define SPIFLASH_SPICON1bits    (SPI1CON1bits)
	#define SPIFLASH_SPICON2        (SPI1CON2)
	#define SPIFLASH_SPISTAT        (SPI1STAT)
	#define SPIFLASH_SPISTATbits    (SPI1STATbits)

	#define ADC_VREF_EN_TRIS		(TRISDbits.TRISD2)
	#define ADC_VREF_EN_IO			(LATDbits.LATD2)
#endif

/*****************************************************************************
 *				--- HARDWARE MAPPING ---					 *
 ****************************************************************************/
	//	Hardware configuration
	#define HWDEFAULT	(1)
	#define	hwdefault	HWDEFAULT 	

	//	IOs
	#define p1					(1)
	#define P1					(1)
	#define p2					(2)
	#define P2					(2)
	#define p3					(3)
	#define P3					(3)
	#define p4					(4)
	#define P4					(4)	
	#define p5					(5)
	#define P5					(5)	
	#define p6					(6)
	#define P6					(6)
	#define p7					(7)
	#define P7					(7)
	#define p8					(8)
	#define P8					(8)
	#define p9					(9)
	#define P9					(9)
	#define p10					(10)
	#define P10					(10)
	#define p11					(11)
	#define P11					(11)
	#define p12					(12)
	#define P12					(12)
	#define p13					(13)
	#define P13					(13)
	#define p14					(14)
	#define P14					(14)
	#define p15					(15)
	#define P15					(15)
    #define p16					(16)
	#define P16					(16)
	#define p17					(17)
	#define P17					(17)
	#define p18					(18)
	#define P18					(18)
	#define p19					(19)
	#define P19					(19)
	#define p20					(20)
	#define P20					(20)
	#define p21					(21)
	#define P21					(21)
    #define p22					(22)
	#define P22					(22)
	#define p23					(23)
	#define P23					(23)
	#define p24					(24)
	#define P24					(24)
	#define p25					(25)
	#define P25					(25)
	#define p26					(26)
	#define P26					(26)
	#define p27					(27)
	#define P27					(27)
	#define p28					(28)
	#define P28					(28)
	#define p29					(29)
	#define P29					(29)
	#define p30					(30)
	#define P30					(30)
	#define p31					(31)
	#define P31					(31)
	#define p32					(32)
	#define P32					(32)
	#define p33					(33)
	#define P33					(33)
	#define p34					(34)
	#define P34					(34)
	#define p35					(35)
	#define P35					(35)
	#define p36					(36)
	#define P36					(36)
	#define p37					(37)
	#define P37					(37)
	#define p38					(38)
	#define P38					(38)
	#define p39					(39)
	#define P39					(39)
		
	//	ADC
#if  defined (FLYPORTGPRS)
	#define ADCANSB		(0x65FC)
#endif
	
	#define ADCEnable	(AD1CON1bits.ADON)
	#define ADCVref		(AD1CON2bits.VCFG)
	
#endif
