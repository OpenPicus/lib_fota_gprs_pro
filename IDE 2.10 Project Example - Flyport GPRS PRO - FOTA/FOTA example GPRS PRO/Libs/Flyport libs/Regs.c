/* **************************************************************************																					
 *                                OpenPicus                 www.openpicus.com
 *                                                            italian concept
 * 
 *            openSource wireless Platform for sensors and Internet of Things	
 * **************************************************************************
 *  FileName:        Regs.c
 *  Dependencies:    Microchip configs files
 *  Module:          FlyPort WI-FI
 *  Compiler:        Microchip C30 v3.12 or higher
 *
  *  Author               Rev.    Date              Comment
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  Gabriele Allegria    1.0     1/20/2011		   First release  (core team)
 *  Simone Marra         2.3    10/22/2012              Added GPRS/3G support
 *						 2.6	10/28/2012			Added GPRS/PRO support
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
#include "GenericTypeDefs.h"
#include "HWmap.h"

#define TRIS_N	(int*) 0x0000
#define TRIS_A	(int*) 0x02C0
#define TRIS_B	(int*) 0x02C8
#define TRIS_C	(int*) 0x02D0
#define TRIS_D	(int*) 0x02D8
#define TRIS_E	(int*) 0x02E0
#define TRIS_F	(int*) 0x02E8
#define TRIS_G	(int*) 0x02F0

#define PORT_N	(int*) 0x0000
#define PORT_A	(int*) 0x02C2
#define PORT_B	(int*) 0x02CA
#define PORT_C	(int*) 0x02D2
#define PORT_D	(int*) 0x02DA
#define PORT_E	(int*) 0x02E2
#define PORT_F	(int*) 0x02EA
#define PORT_G	(int*) 0x02F2

#define LAT_N	(int*) 0x0000
#define LAT_A	(int*) 0x02C4
#define LAT_B	(int*) 0x02CC
#define LAT_C	(int*) 0x02D4
#define LAT_D	(int*) 0x02DC
#define LAT_E	(int*) 0x02E4
#define LAT_F	(int*) 0x02EC
#define LAT_G	(int*) 0x02F4


#if defined (FLYPORT_G)

int *LATs[] 	= 	{	
						LAT_B,
						LAT_N,
						LAT_B,
						LAT_N,
						LAT_B,
						LAT_N,
						LAT_B,
						LAT_B,
						LAT_F,
						LAT_B,
						LAT_F,
						LAT_B,
						LAT_D,
						LAT_D,
						LAT_D,
						LAT_B,
						LAT_D,
						LAT_B,
						LAT_D,
						LAT_B,
						LAT_D,
						LAT_B,
						LAT_D,
						LAT_F,
						LAT_G,
						LAT_F,
						LAT_G,
						LAT_F,
						LAT_N,/*PIC pin35, Vusb, not an I/O... - LAT_F,*/
						LAT_F,
						LAT_E,
						LAT_E,
						LAT_E,
						LAT_E,
						LAT_E,
						LAT_E,
						LAT_E,
						LAT_E,
						LAT_D,
						LAT_D
					};

int *TRISs[] 	= 	{	
						TRIS_B,
						TRIS_N,
						TRIS_B,
						TRIS_N,
						TRIS_B,
						TRIS_N,
						TRIS_B,
						TRIS_B,
						TRIS_F,
						TRIS_B,
						TRIS_F,
						TRIS_B,
						TRIS_D,
						TRIS_D,
						TRIS_D,
						TRIS_B,
						TRIS_D,
						TRIS_B,
						TRIS_D,
						TRIS_B,
						TRIS_D,
						TRIS_B,
						TRIS_D,
						TRIS_F,
						TRIS_G,
						TRIS_F,
						TRIS_G,
						TRIS_F,
						TRIS_N, /*PIC pin35, Vusb, not an I/O... - TRIS_F,*/
						TRIS_F,
						TRIS_E,
						TRIS_E,
						TRIS_E,
						TRIS_E,
						TRIS_E,
						TRIS_E,
						TRIS_E,
						TRIS_E,
						TRIS_D,
						TRIS_D
					};
						
int *PORTs[] 	= 	{	
						PORT_B,
						PORT_N,
						PORT_B,
						PORT_N,
						PORT_B,
						PORT_N,
						PORT_B,
						PORT_B,
						PORT_F,
						PORT_B,
						PORT_F,
						PORT_B,
						PORT_D,
						PORT_D,
						PORT_D,
						PORT_B,
						PORT_D,
						PORT_B,
						PORT_D,
						PORT_B,
						PORT_D,
						PORT_B,
						PORT_D,
						PORT_F,
						PORT_G,
						PORT_F,
						PORT_G,
						PORT_F,
						PORT_N, /*PIC pin35, Vusb, not an I/O... - PORT_F,*/
						PORT_F,
						PORT_E,
						PORT_E,
						PORT_E,
						PORT_E,
						PORT_E,
						PORT_E,
						PORT_E,
						PORT_E,
						PORT_D,
						PORT_D
					};

int *CNPUs[]	=	{	(int*) 0x6E , (int*) 0x00 , (int*) 0x6E , 
						(int*) 0x00 , (int*) 0x6E , (int*) 0x00 , 
						(int*) 0x6E , (int*) 0x70 , (int*) 0x76 , 
						(int*) 0x70 , (int*) 0x76 , (int*) 0x70 , 
						(int*) 0x74 , (int*) 0x6E , (int*) 0x74 , 
						(int*) 0x70 , (int*) 0x74 , (int*) 0x70 ,
						(int*) 0x74 , (int*) 0x72 , (int*) 0x74 ,
						(int*) 0x6E , (int*) 0x6E , (int*) 0x70 ,
						(int*) 0x78 , (int*) 0x70 , (int*) 0x78 ,
						(int*) 0x76 , (int*) 0x00 , (int*) 0x00 ,
						(int*) 0x76 , (int*) 0x76 , (int*) 0x74 ,
						(int*) 0x74 , (int*) 0x74 , (int*) 0x74 ,
						(int*) 0x74 , (int*) 0x74 , (int*) 0x74 ,
						(int*) 0x74
						};
						
int *CNPDs[]	=	{	(int*) 0x56 , (int*) 0x00 , (int*) 0x56 , 
						(int*) 0x00 , (int*) 0x56 , (int*) 0x00 , 
						(int*) 0x56 , (int*) 0x58 , (int*) 0x5E , 
						(int*) 0x58 , (int*) 0x5E , (int*) 0x58 , 
						(int*) 0x5C , (int*) 0x56 , (int*) 0x5C , 
						(int*) 0x58 , (int*) 0x5C , (int*) 0x58 ,
						(int*) 0x5C , (int*) 0x5A , (int*) 0x5C ,
						(int*) 0x56 , (int*) 0x56 , (int*) 0x58 ,
						(int*) 0x60 , (int*) 0x58 , (int*) 0x60 ,
						(int*) 0x5E , (int*) 0x00 , (int*) 0x00 ,
						(int*) 0x5E , (int*) 0x5E , (int*) 0x5C ,
						(int*) 0x5C , (int*) 0x5C , (int*) 0x5C ,
						(int*) 0x5C , (int*) 0x5C , (int*) 0x5C ,
						(int*) 0x5C
						};
						
int IOMode[40];

int CNPos[]		=	{	7 , -1, 6 , -1, 5 , -1, 4 , 9 , 4 , 8 ,
						5 , 10, 2 , 14, 1 , 12, 8 , 15, 6 , 0 , 
						7 , 12, 13, 1 , 3 , 2 , 4 , 7 , -1, -1, 
						1 , 0 , 15, 14, 13, 12, 11, 10, 3 , 4 
					};

int IOPos[] 	= 	{	5 , -1 , 4 , -1 , 3 , -1 , 2 , 7 , 0 , 6 ,
						1 , 8 , 1 , 5 , 0 , 10 , 11 , 13 , 9 , 14 ,
						10 , 15 , 4 , 4 , 2 , 5 , 3 , 3 , 6 , 2 ,
						7 , 6 , 5 , 4 , 3 , 2 , 1 , 0 , 2 , 3
					};

int an[] = {5, 4, 3, 2, 6, 7, 8, 10, 13, 14};

int *UMODEs[]	=	{	(int*) 0x220 , (int*) 0x230 , (int*) 0x250 , (int*) 0x2B0};
int *USTAs[]	=	{	(int*) 0x222 , (int*) 0x232 , (int*) 0x252 , (int*) 0x2B2};
int *UBRGs[]	=	{	(int*) 0x228 , (int*) 0x238 , (int*) 0x258 , (int*) 0x2B8};
int *UIFSs[]	=	{	(int*) 0x084 , (int*) 0x086 , (int*) 0x08E , (int*) 0x08E};
int *UIECs[]	=	{	(int*) 0x094 , (int*) 0x096 , (int*) 0x09E , (int*) 0x09E};
int *UTXREGs[]	=	{	(int*) 0x224 , (int*) 0x234 , (int*) 0x254 , (int*) 0x2B4};
int *URXREGs[]	=	{	(int*) 0x226 , (int*) 0x236 , (int*) 0x256 , (int*) 0x2B6};

int UTXIPos[]	=	{	4096 , 32768 , 8 , 512};
int URXIPos[]	=	{	2048 , 16384 , 4 , 256};						

//int *AD1CFGL  	=	(int*) 0x032C;
//int *AD1CFGH  	=	(int*) 0x032A;
int *AD1CONF1 	=	(int*) 0x0320;
int *AD1CONF2 	=	(int*) 0x0322;
int *AD1CONF3 	=	(int*) 0x0324;
int *AD1CH 	  	=	(int*) 0x0328;
int *AD1CSL   	=	(int*) 0x0330;

int *OCCON1s[]  =	{  	(int*) 0x0190, (int*) 0x019A, (int*) 0x01A4,
						(int*) 0x01AE, (int*) 0x01B8, (int*) 0x01C2,
						(int*) 0x01CC, (int*) 0x01D6, (int*) 0x01E0  };   


int *OCCON2s[]  =	{  	(int*) 0x0192, (int*) 0x019C, (int*) 0x01A6,
						(int*) 0x01B0, (int*) 0x01BA, (int*) 0x01C4,
						(int*) 0x01CE, (int*) 0x01D8, (int*) 0x01E2  };


int *OCRs[]     = 	{  	(int*) 0x0196, (int*) 0x01A0, (int*) 0x01AA,
						(int*) 0x01B4, (int*) 0x01BE, (int*) 0x01C8,
						(int*) 0x01D2, (int*) 0x01DC, (int*) 0x01E6  };


int *OCRSs[]    = 	{  	(int*) 0x0194, (int*) 0x019E, (int*) 0x01A8,
						(int*) 0x01B2, (int*) 0x01BC, (int*) 0x01C6,
						(int*) 0x01D0, (int*) 0x01DA, (int*) 0x01E4  };


int *RPORs[]    =	{  	(int*) 0x06D2, (int*) 0x0000, (int*) 0x06DC,
						(int*) 0x0000, (int*) 0x0000, (int*) 0x0000,
						(int*) 0x06CC, (int*) 0x06C6, (int*) 0x0000,
						(int*) 0x06C6, (int*) 0x0000, (int*) 0x06C8,
						(int*) 0x06D8, (int*) 0x06D4, (int*) 0x06CA,
						(int*) 0x0000, (int*) 0x06CC, (int*) 0x0000,
						(int*) 0x06C4, (int*) 0x06CE, (int*) 0x06C2,
						(int*) 0x06DC, (int*) 0x06D8, (int*) 0x06CA,
						(int*) 0x0000, (int*) 0x06D0, (int*) 0x0000,
						(int*) 0x06D0, (int*) 0x0000, (int*) 0x0000,
						(int*) 0x0000, (int*) 0x0000, (int*) 0x0000,
						(int*) 0x0000, (int*) 0x0000, (int*) 0x0000, 
						(int*) 0x0000, (int*) 0x0000, (int*) 0x06D6, 
						(int*) 0x06D6 	
					};

int RPFunc[]	=	{	3, 4, 5, 6, 28, 29, 30, 31, 11, 10, 12	};

int RPIORPin[]  =  	{  18,  0, 28,  0,  0,  0, 13,  7,  0,  6, 
						0,  8, 24, 20, 11,  0, 12,  0,  4, 14, 
						3, 29, 25, 10,  0, 17,  0, 16,  0,  0, 
						0,  0,  0,  0,  0,  0,  0,  0, 23, 22
					};

int *TCONs[]	=	{	(int*) 0x0104 , (int*) 0x0110 , (int*) 0x0112 , 
						(int*) 0x011E , (int*) 0x0120	};
					  
int OCM[]       =  	{ 18, 19, 20, 21, 22, 23, 24, 25, 35 };	

int *RPIRs[]	=	{	(int*) 0x06A4, (int*) 0x06A4, (int*) 0x06A6, 
						(int*) 0x06A6, (int*) 0x06A2, (int*) 0x06AA,
						(int*) 0x06B6, (int*) 0x06B6, (int*) 0x0682,
						(int*) 0x0682, (int*) 0x0684, (int*) 0x06AC,
						(int*) 0x06AC, (int*) 0x06AE, (int*) 0x0688
					};
						

BOOL RPIRPos[]	=	{	0, 1, 0, 
						1, 1, 1,
						0, 1, 0,
						1, 0, 1,
						0, 0, 0
					};
#endif

#if defined FLYPORT_ETH

int *LATs[] 	= 	{	
						LAT_B,
						LAT_N,
						LAT_B,
						LAT_N,
						LAT_B,
						LAT_N,
						LAT_B,
						LAT_B,
						LAT_F,
						LAT_B,
						LAT_F,
						LAT_B,
						LAT_D,
						LAT_D,
						LAT_D,
						LAT_B,
						LAT_D,
						LAT_B,
						LAT_D,
						LAT_B,
						LAT_D,
						LAT_B,
						LAT_D,
						LAT_F,
						LAT_G,
						LAT_F,
						LAT_G,
						LAT_F,
						LAT_N,/*PIC pin35, Vusb, not an I/O... - LAT_F,*/
						LAT_F,
						LAT_E,
						LAT_E,
						LAT_E,
						LAT_E,
						LAT_E,
						LAT_E,
						LAT_E,
						LAT_E,
						LAT_D,
						LAT_D
					};

int *TRISs[] 	= 	{	
						TRIS_B,
						TRIS_N,
						TRIS_B,
						TRIS_N,
						TRIS_B,
						TRIS_N,
						TRIS_B,
						TRIS_B,
						TRIS_F,
						TRIS_B,
						TRIS_F,
						TRIS_B,
						TRIS_D,
						TRIS_D,
						TRIS_D,
						TRIS_B,
						TRIS_D,
						TRIS_B,
						TRIS_D,
						TRIS_B,
						TRIS_D,
						TRIS_B,
						TRIS_D,
						TRIS_F,
						TRIS_G,
						TRIS_F,
						TRIS_G,
						TRIS_F,
						TRIS_N, /*PIC pin35, Vusb, not an I/O... - TRIS_F,*/
						TRIS_F,
						TRIS_E,
						TRIS_E,
						TRIS_E,
						TRIS_E,
						TRIS_E,
						TRIS_E,
						TRIS_E,
						TRIS_E,
						TRIS_D,
						TRIS_D
					};
						
int *PORTs[] 	= 	{	
						PORT_B,
						PORT_N,
						PORT_B,
						PORT_N,
						PORT_B,
						PORT_N,
						PORT_B,
						PORT_B,
						PORT_F,
						PORT_B,
						PORT_F,
						PORT_B,
						PORT_D,
						PORT_D,
						PORT_D,
						PORT_B,
						PORT_D,
						PORT_B,
						PORT_D,
						PORT_B,
						PORT_D,
						PORT_B,
						PORT_D,
						PORT_F,
						PORT_G,
						PORT_F,
						PORT_G,
						PORT_F,
						PORT_N, /*PIC pin35, Vusb, not an I/O... - PORT_F,*/
						PORT_F,
						PORT_E,
						PORT_E,
						PORT_E,
						PORT_E,
						PORT_E,
						PORT_E,
						PORT_E,
						PORT_E,
						PORT_D,
						PORT_D
					};

int *CNPUs[]	=	{	(int*) 0x6E , (int*) 0x00 , (int*) 0x6E , 
						(int*) 0x00 , (int*) 0x6E , (int*) 0x00 , 
						(int*) 0x6E , (int*) 0x70 , (int*) 0x76 , 
						(int*) 0x70 , (int*) 0x76 , (int*) 0x70 , 
						(int*) 0x74 , (int*) 0x6E , (int*) 0x74 , 
						(int*) 0x70 , (int*) 0x74 , (int*) 0x70 ,
						(int*) 0x74 , (int*) 0x72 , (int*) 0x74 ,
						(int*) 0x6E , (int*) 0x6E , (int*) 0x70 ,
						(int*) 0x78 , (int*) 0x70 , (int*) 0x78 ,
						(int*) 0x76 , (int*) 0x00 , (int*) 0x00 ,
						(int*) 0x76 , (int*) 0x76 , (int*) 0x74 ,
						(int*) 0x74 , (int*) 0x74 , (int*) 0x74 ,
						(int*) 0x74 , (int*) 0x74 , (int*) 0x74 ,
						(int*) 0x74
						};
						
int *CNPDs[]	=	{	(int*) 0x56 , (int*) 0x00 , (int*) 0x56 , 
						(int*) 0x00 , (int*) 0x56 , (int*) 0x00 , 
						(int*) 0x56 , (int*) 0x58 , (int*) 0x5E , 
						(int*) 0x58 , (int*) 0x5E , (int*) 0x58 , 
						(int*) 0x5C , (int*) 0x56 , (int*) 0x5C , 
						(int*) 0x58 , (int*) 0x5C , (int*) 0x58 ,
						(int*) 0x5C , (int*) 0x5A , (int*) 0x5C ,
						(int*) 0x56 , (int*) 0x56 , (int*) 0x58 ,
						(int*) 0x60 , (int*) 0x58 , (int*) 0x60 ,
						(int*) 0x5E , (int*) 0x00 , (int*) 0x00 ,
						(int*) 0x5E , (int*) 0x5E , (int*) 0x5C ,
						(int*) 0x5C , (int*) 0x5C , (int*) 0x5C ,
						(int*) 0x5C , (int*) 0x5C , (int*) 0x5C ,
						(int*) 0x5C
						};
						
int IOMode[40];

int CNPos[]		=	{	7 , -1, 6 , -1, 5 , -1, 4 , 9 , 4 , 8 ,
						5 , 10, 2 , 14, 1 , 12, 8 , 15, 6 , 0 , 
						7 , 12, 13, 1 , 3 , 2 , 4 , 7 , -1, -1, 
						1 , 0 , 15, 14, 13, 12, 11, 10, 3 , 4 
					};

int IOPos[] 	= 	{	5 , -1 , 4 , -1 , 3 , -1 , 2 , 7 , 0 , 6 ,
						1 , 8 , 1 , 5 , 0 , 10 , 11 , 13 , 9 , 14 ,
						10 , 15 , 4 , 4 , 2 , 5 , 3 , 3 , 6 , 2 ,
						7 , 6 , 5 , 4 , 3 , 2 , 1 , 0 , 2 , 3
					};

int an[] = {5, 4, 3, 2, 6, 7, 8, 10, 13, 14};

int *UMODEs[]	=	{	(int*) 0x220 , (int*) 0x230 , (int*) 0x250 , (int*) 0x2B0};
int *USTAs[]	=	{	(int*) 0x222 , (int*) 0x232 , (int*) 0x252 , (int*) 0x2B2};
int *UBRGs[]	=	{	(int*) 0x228 , (int*) 0x238 , (int*) 0x258 , (int*) 0x2B8};
int *UIFSs[]	=	{	(int*) 0x084 , (int*) 0x086 , (int*) 0x08E , (int*) 0x08E};
int *UIECs[]	=	{	(int*) 0x094 , (int*) 0x096 , (int*) 0x09E , (int*) 0x09E};
int *UTXREGs[]	=	{	(int*) 0x224 , (int*) 0x234 , (int*) 0x254 , (int*) 0x2B4};
int *URXREGs[]	=	{	(int*) 0x226 , (int*) 0x236 , (int*) 0x256 , (int*) 0x2B6};

int UTXIPos[]	=	{	4096 , 32768 , 8 , 512};
int URXIPos[]	=	{	2048 , 16384 , 4 , 256};						

//int *AD1CFGL  	=	(int*) 0x032C;
//int *AD1CFGH  	=	(int*) 0x032A;
int *AD1CONF1 	=	(int*) 0x0320;
int *AD1CONF2 	=	(int*) 0x0322;
int *AD1CONF3 	=	(int*) 0x0324;
int *AD1CH 	  	=	(int*) 0x0328;
int *AD1CSL   	=	(int*) 0x0330;

int *OCCON1s[]  =	{  	(int*) 0x0190, (int*) 0x019A, (int*) 0x01A4,
						(int*) 0x01AE, (int*) 0x01B8, (int*) 0x01C2,
						(int*) 0x01CC, (int*) 0x01D6, (int*) 0x01E0  };   


int *OCCON2s[]  =	{  	(int*) 0x0192, (int*) 0x019C, (int*) 0x01A6,
						(int*) 0x01B0, (int*) 0x01BA, (int*) 0x01C4,
						(int*) 0x01CE, (int*) 0x01D8, (int*) 0x01E2  };


int *OCRs[]     = 	{  	(int*) 0x0196, (int*) 0x01A0, (int*) 0x01AA,
						(int*) 0x01B4, (int*) 0x01BE, (int*) 0x01C8,
						(int*) 0x01D2, (int*) 0x01DC, (int*) 0x01E6  };


int *OCRSs[]    = 	{  	(int*) 0x0194, (int*) 0x019E, (int*) 0x01A8,
						(int*) 0x01B2, (int*) 0x01BC, (int*) 0x01C6,
						(int*) 0x01D0, (int*) 0x01DA, (int*) 0x01E4  };


int *RPORs[]    =	{  	(int*) 0x06D2, (int*) 0x0000, (int*) 0x06DC,
						(int*) 0x0000, (int*) 0x0000, (int*) 0x0000,
						(int*) 0x06CC, (int*) 0x06C6, (int*) 0x0000,
						(int*) 0x06C6, (int*) 0x0000, (int*) 0x06C8,
						(int*) 0x06D8, (int*) 0x06D4, (int*) 0x06CA,
						(int*) 0x0000, (int*) 0x06CC, (int*) 0x0000,
						(int*) 0x06C4, (int*) 0x06CE, (int*) 0x06C2,
						(int*) 0x06DC, (int*) 0x06D8, (int*) 0x06CA,
						(int*) 0x0000, (int*) 0x06D0, (int*) 0x0000,
						(int*) 0x06D0, (int*) 0x0000, (int*) 0x0000,
						(int*) 0x0000, (int*) 0x0000, (int*) 0x0000,
						(int*) 0x0000, (int*) 0x0000, (int*) 0x0000, 
						(int*) 0x0000, (int*) 0x0000, (int*) 0x06D6, 
						(int*) 0x06D6 	
					};

int RPFunc[]	=	{	3, 4, 5, 6, 28, 29, 30, 31, 11, 10, 12	};

int RPIORPin[]  =  	{  18,  0, 28,  0,  0,  0, 13,  7,  0,  6, 
						0,  8, 24, 20, 11,  0, 12,  0,  4, 14, 
						3, 29, 25, 10,  0, 17,  0, 16,  0,  0, 
						0,  0,  0,  0,  0,  0,  0,  0, 23, 22
					};

int *TCONs[]	=	{	(int*) 0x0104 , (int*) 0x0110 , (int*) 0x0112 , 
						(int*) 0x011E , (int*) 0x0120	};
					  
int OCM[]       =  	{ 18, 19, 20, 21, 22, 23, 24, 25, 35 };	

int *RPIRs[]	=	{	(int*) 0x06A4, (int*) 0x06A4, (int*) 0x06A6, 
						(int*) 0x06A6, (int*) 0x06A2, (int*) 0x06AA,
						(int*) 0x06B6, (int*) 0x06B6, (int*) 0x0682,
						(int*) 0x0682, (int*) 0x0684, (int*) 0x06AC,
						(int*) 0x06AC, (int*) 0x06AE, (int*) 0x0688
					};
						

BOOL RPIRPos[]	=	{	0, 1, 0, 
						1, 1, 1,
						0, 1, 0,
						1, 0, 1,
						0, 0, 0
					};
#endif

#if defined (FLYPORTGPRS)

int *LATs[] 	= 	{	
						LAT_B,
						LAT_N,
						LAT_B,
						LAT_N,
						LAT_B,
						LAT_N,
						LAT_B,
						LAT_B,
						LAT_F,
						LAT_B,
						LAT_F,
						LAT_B,
						LAT_D,
						LAT_D,
						LAT_D,
						LAT_B,
						LAT_D,
						LAT_B,
						LAT_D,
						LAT_B,
						LAT_D,
						LAT_B,
						LAT_D,
						LAT_F,
						LAT_G,
						LAT_F,
						LAT_G,
						LAT_F,
						LAT_N,/*PIC pin35, Vusb, not an I/O... - LAT_F,*/
						LAT_F,
						LAT_E,
						LAT_E,
						LAT_E,
						LAT_E,
						LAT_E,
						LAT_N,
						LAT_N,
						LAT_N,
						LAT_N,
						LAT_N
					};

int *TRISs[] 	= 	{	
						TRIS_B,
						TRIS_N,
						TRIS_B,
						TRIS_N,
						TRIS_B,
						TRIS_N,
						TRIS_B,
						TRIS_B,
						TRIS_F,
						TRIS_B,
						TRIS_F,
						TRIS_B,
						TRIS_D,
						TRIS_D,
						TRIS_D,
						TRIS_B,
						TRIS_D,
						TRIS_B,
						TRIS_D,
						TRIS_B,
						TRIS_D,
						TRIS_B,
						TRIS_D,
						TRIS_F,
						TRIS_G,
						TRIS_F,
						TRIS_G,
						TRIS_F,
						TRIS_N, /*PIC pin35, Vusb, not an I/O... - TRIS_F,*/
						TRIS_F,
						TRIS_E,
						TRIS_E,
						TRIS_E,
						TRIS_E,
						TRIS_E,
						TRIS_N,
						TRIS_N,
						TRIS_N,
						TRIS_N,
						TRIS_N
					};
						
int *PORTs[] 	= 	{	
						PORT_B,
						PORT_N,
						PORT_B,
						PORT_N,
						PORT_B,
						PORT_N,
						PORT_B,
						PORT_B,
						PORT_F,
						PORT_B,
						PORT_F,
						PORT_B,
						PORT_D,
						PORT_D,
						PORT_D,
						PORT_B,
						PORT_D,
						PORT_B,
						PORT_D,
						PORT_B,
						PORT_D,
						PORT_B,
						PORT_D,
						PORT_F,
						PORT_G,
						PORT_F,
						PORT_G,
						PORT_F,
						PORT_N, /*PIC pin35, Vusb, not an I/O... - PORT_F,*/
						PORT_F,
						PORT_E,
						PORT_E,
						PORT_E,
						PORT_E,
						PORT_E,
						PORT_N,
						PORT_N,
						PORT_N,
						PORT_N,
						PORT_N
					};

int *CNPUs[]	=	{	(int*) 0x6E , (int*) 0x00 , (int*) 0x6E , 
						(int*) 0x00 , (int*) 0x6E , (int*) 0x00 , 
						(int*) 0x6E , (int*) 0x70 , (int*) 0x76 , 
						(int*) 0x70 , (int*) 0x76 , (int*) 0x70 , 
						(int*) 0x74 , (int*) 0x6E , (int*) 0x74 , 
						(int*) 0x70 , (int*) 0x74 , (int*) 0x70 ,
						(int*) 0x74 , (int*) 0x72 , (int*) 0x74 ,
						(int*) 0x6E , (int*) 0x6E , (int*) 0x70 ,
						(int*) 0x78 , (int*) 0x70 , (int*) 0x78 ,
						(int*) 0x76 , (int*) 0x00 , (int*) 0x00 ,
						(int*) 0x76 , (int*) 0x76 , (int*) 0x74 ,
						(int*) 0x74 , (int*) 0x74 , (int*) 0x74 ,
						(int*) 0x74 , (int*) 0x74 , (int*) 0x74 ,
						(int*) 0x74
						};
						
int *CNPDs[]	=	{	(int*) 0x56 , (int*) 0x00 , (int*) 0x56 , 
						(int*) 0x00 , (int*) 0x56 , (int*) 0x00 , 
						(int*) 0x56 , (int*) 0x58 , (int*) 0x5E , 
						(int*) 0x58 , (int*) 0x5E , (int*) 0x58 , 
						(int*) 0x5C , (int*) 0x56 , (int*) 0x5C , 
						(int*) 0x58 , (int*) 0x5C , (int*) 0x58 ,
						(int*) 0x5C , (int*) 0x5A , (int*) 0x5C ,
						(int*) 0x56 , (int*) 0x56 , (int*) 0x58 ,
						(int*) 0x60 , (int*) 0x58 , (int*) 0x60 ,
						(int*) 0x5E , (int*) 0x00 , (int*) 0x00 ,
						(int*) 0x5E , (int*) 0x5E , (int*) 0x5C ,
						(int*) 0x5C , (int*) 0x5C , (int*) 0x5C ,
						(int*) 0x5C , (int*) 0x5C , (int*) 0x5C ,
						(int*) 0x5C
						};
						
int IOMode[40];

int CNPos[]		=	{	7 , -1, 6 , -1, 5 , -1, 4 , 9 , 4 , 8 ,
						5 , 10, 2 , 14, 1 , 12, 8 , 15, 6 , 0 , 
						7 , 12, 13, 1 , 3 , 2 , 4 , 7 , -1, -1, 
						1 , 0 , 15, 14, 13, -1, -1, -1, -1, -1 
					};

int IOPos[] 	= 	{	5 , -1, 4 , -1, 3 , -1, 2 , 7 , 0 , 6 ,
						1 , 8 , 1 , 5 , 0 , 10, 11, 13, 9 , 14,
						10, 15, 4 , 4 , 2 , 5 , 3 , 3 , -1, 7 ,
						7 , 6 , 5 , 4 , 3 , -1, -1, -1, -1, -1
					};

int an[] = {5, 4, 3, 2, 6, 7, 8, 10, 13, 14};

int *UMODEs[]	=	{	(int*) 0x220 , (int*) 0x230 , (int*) 0x250 , (int*) 0x2B0};
int *USTAs[]	=	{	(int*) 0x222 , (int*) 0x232 , (int*) 0x252 , (int*) 0x2B2};
int *UBRGs[]	=	{	(int*) 0x228 , (int*) 0x238 , (int*) 0x258 , (int*) 0x2B8};
int *UIFSs[]	=	{	(int*) 0x084 , (int*) 0x086 , (int*) 0x08E , (int*) 0x08E};
int *UIECs[]	=	{	(int*) 0x094 , (int*) 0x096 , (int*) 0x09E , (int*) 0x09E};
int *UTXREGs[]	=	{	(int*) 0x224 , (int*) 0x234 , (int*) 0x254 , (int*) 0x2B4};
int *URXREGs[]	=	{	(int*) 0x226 , (int*) 0x236 , (int*) 0x256 , (int*) 0x2B6};

int UTXIPos[]	=	{	4096 , 32768 , 8 , 512};
int URXIPos[]	=	{	2048 , 16384 , 4 , 256};						

//int *AD1CFGL  	=	(int*) 0x032C;
//int *AD1CFGH  	=	(int*) 0x032A;
int *AD1CONF1 	=	(int*) 0x0320;
int *AD1CONF2 	=	(int*) 0x0322;
int *AD1CONF3 	=	(int*) 0x0324;
int *AD1CH 	  	=	(int*) 0x0328;
int *AD1CSL   	=	(int*) 0x0330;

int *OCCON1s[]  =	{  	(int*) 0x0190, (int*) 0x019A, (int*) 0x01A4,
						(int*) 0x01AE, (int*) 0x01B8, (int*) 0x01C2,
						(int*) 0x01CC, (int*) 0x01D6, (int*) 0x01E0  };   


int *OCCON2s[]  =	{  	(int*) 0x0192, (int*) 0x019C, (int*) 0x01A6,
						(int*) 0x01B0, (int*) 0x01BA, (int*) 0x01C4,
						(int*) 0x01CE, (int*) 0x01D8, (int*) 0x01E2  };


int *OCRs[]     = 	{  	(int*) 0x0196, (int*) 0x01A0, (int*) 0x01AA,
						(int*) 0x01B4, (int*) 0x01BE, (int*) 0x01C8,
						(int*) 0x01D2, (int*) 0x01DC, (int*) 0x01E6  };


int *OCRSs[]    = 	{  	(int*) 0x0194, (int*) 0x019E, (int*) 0x01A8,
						(int*) 0x01B2, (int*) 0x01BC, (int*) 0x01C6,
						(int*) 0x01D0, (int*) 0x01DA, (int*) 0x01E4  };


int *RPORs[]    =	{  	(int*) 0x06D2, (int*) 0x0000, (int*) 0x06DC,
						(int*) 0x0000, (int*) 0x0000, (int*) 0x0000,
						(int*) 0x06CC, (int*) 0x06C6, (int*) 0x0000,
						(int*) 0x06C6, (int*) 0x0000, (int*) 0x06C8,
						(int*) 0x06D8, (int*) 0x06D4, (int*) 0x06CA,
						(int*) 0x0000, (int*) 0x06CC, (int*) 0x0000,
						(int*) 0x06C4, (int*) 0x06CE, (int*) 0x06C2,
						(int*) 0x06DC, (int*) 0x06D8, (int*) 0x06CA,
						(int*) 0x0000, (int*) 0x06D0, (int*) 0x0000,
						(int*) 0x06D0, (int*) 0x0000, (int*) 0x0000,
						(int*) 0x0000, (int*) 0x0000, (int*) 0x0000,
						(int*) 0x0000, (int*) 0x0000, (int*) 0x0000, 
						(int*) 0x0000, (int*) 0x0000, (int*) 0x0000, 
						(int*) 0x0000 	
					};

int RPFunc[]	=	{	3, 4, 5, 6, 28, 29, 30, 31, 11, 10, 12	};

int RPIORPin[]  =  	{  18,  0, 28,  0,  0,  0, 13,  7,  0,  6, 
						0,  8, 24, 20, 11,  0, 12,  0,  4, 14,
						3, 29, 25, 10,  0, 17,  0, 16,  0,  0, 
						0,  0,  0,  0,  0,  0,  0,  0,  0,  0
					};

int *TCONs[]	=	{	(int*) 0x0104 , (int*) 0x0110 , (int*) 0x0112 , 
						(int*) 0x011E , (int*) 0x0120	};
					  
int OCM[]       =  	{ 18, 19, 20, 21, 22, 23, 24, 25, 35 };	

int *RPIRs[]	=	{	(int*) 0x06A4, (int*) 0x06A4, (int*) 0x06A6, 
						(int*) 0x06A6, (int*) 0x06A2, (int*) 0x06AA,
						(int*) 0x06B6, (int*) 0x06B6, (int*) 0x0682,
						(int*) 0x0682, (int*) 0x0684, (int*) 0x06AC,
						(int*) 0x06AC, (int*) 0x06AE, (int*) 0x0688
					};
						

BOOL RPIRPos[]	=	{	0, 1, 0, 
						1, 1, 1,
						0, 1, 0,
						1, 0, 1,
						0, 0, 0
					};
#endif
