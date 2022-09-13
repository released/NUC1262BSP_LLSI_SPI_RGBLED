
/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

/*_____ D E C L A R A T I O N S ____________________________________________*/

/*_____ D E F I N I T I O N S ______________________________________________*/


/*
	[LED]
	SPI0 port:
	MOSI	:PA0
	CLK		:PA2
	SS		:PA3

	SPI1 port:
	MOSI	:PB4
	CLK		:PB3
	SS		:PB2	

	LLSI0	:PC5
	LLSI1	:PC4
	LLSI2	:PC3
	LLSI3	:PC2

*/

#define LED_STRIPE_IDX_0							(0)
#define LED_STRIPE_IDX_1							(1)

#define LED_STRIPE_IDX_2							(2)
#define LED_STRIPE_IDX_3							(3)
#define LED_STRIPE_IDX_4							(4)
#define LED_STRIPE_IDX_5							(5)

#define LED_PORT0									(SPI0)
#define LED_PORT0_PDMA_CH   						(2)
#define LED_PORT0_PDMA_OPENED_CH   					(1 << LED_PORT0_PDMA_CH)

#define LED_PORT1									(SPI1)
#define LED_PORT1_PDMA_CH   						(3)
#define LED_PORT1_PDMA_OPENED_CH   					(1 << LED_PORT1_PDMA_CH)

#define LED_PORT2									(LLSI0)
#define LED_PORT2_PDMA_CH   						(4)
#define LED_PORT2_PDMA_OPENED_CH   					(1 << LED_PORT2_PDMA_CH)

#define LED_PORT3									(LLSI1)
#define LED_PORT3_PDMA_CH   						(5)
#define LED_PORT3_PDMA_OPENED_CH   					(1 << LED_PORT3_PDMA_CH)

#define LED_PORT4									(LLSI2)
#define LED_PORT4_PDMA_CH   						(6)
#define LED_PORT4_PDMA_OPENED_CH   					(1 << LED_PORT4_PDMA_CH)

#define LED_PORT5									(LLSI3)
#define LED_PORT5_PDMA_CH   						(7)
#define LED_PORT5_PDMA_OPENED_CH   					(1 << LED_PORT5_PDMA_CH)


/*
	EXAMPLE : 
	6 LED , each LED with 3 bytes (RGB) , total target 18 BYTES
	by using uin32_t arry output (4 bytes) , 18 / 4 = 4.5 , need send 4.5 x uint32_t = 5 x uint32_t

	24 LED , each LED with 3 bytes (RGB) , total target 24 X 3 BYTES
	by using uin32_t arry output (4 bytes) , 72 / 4 = 18 , need send 18 x uint32_t

	6 LEDs = 6 x 3 (RGB color) = 18 bytes , need to use 5 x uin32_t buffer ( 18 / 4 = 4.5)		
	8 LEDs = 8 x 3 (RGB color) = 24 bytes , need to use 6 x uin32_t buffer ( 24 / 4 = 6)
	24 LEDs = 24 x 3 (RGB color) = 72 bytes , need to use 18 x uin32_t buffer ( 72 / 4 = 18)	

	64 LEDs = 64 x 3 (RGB color) = 192 bytes , need to use 48 x uin32_t buffer ( 192 / 4 = 48)	

	100 LEDs = 100 x 3 (RGB color) = 300 bytes , need to use 75 x uin32_t buffer ( 300 / 4 = 75)
	300 LEDs = 300 x 3 (RGB color) = 900 bytes , need to use 225 x uin32_t buffer ( 900 / 4 = 225)	
	
*/

//debug purpose

/*
	if use ENABLE_DEBUG_WS2812_PROTOCOL , 
	need to set LED_NUM : 6 , LED_DATA_LEN : 5
*/
// #define ENABLE_DEBUG_WS2812_PROTOCOL

#if defined (ENABLE_DEBUG_WS2812_PROTOCOL)
//#define LED_NUM									(6)
//#define LED_DATA_LEN								(5)

#define LED_NUM										(300)
#define LED_SPI_DATA_LEN							(LED_NUM*3)
#define LED_LLSI_DATA_LEN							(225)

#else	// custom

//WS2813 , 60 LEDs/meter , total 5 meters

#define LED_NUM										(300)
#define LED_SPI_DATA_LEN							(LED_NUM*3)
#define LED_LLSI_DATA_LEN							(LED_NUM*3)	//(225)

// for test
// #define LED_NUM										(100)
// #define LED_SPI_DATA_LEN							(LED_NUM*3)
// #define LED_LLSI_DATA_LEN							(75)

#endif

/*
	WS2812
	T1H 1 HIGH 580ns~1.6us		target : 0.750 us
	T1L 1 LOW 220ns~420ns		target : 0.375 us
	==> 1111 1100 (0xFC)	==> if SPI = 7M
	
	T0H 0 HIGH 220ns~380ns		target : 0.375 us
	T0L 0 LOW 580ns~1.6us		target : 0.750 us
	==> 1100 0000 (0xC0)	==> if SPI = 7M	
 	_____   
	|     |___|   1111 1100  high level
 	___         
	|   |_____|   1100 0000  low level
	
	RES : more than 280us

	WS2813 : TH+TL=1.25usÅ}300ns , RGB
	T0H : 300ns ~ 450ns
	T1H : 750ns ~ 1000ns
	T0L : 300ns ~ 100us
	T1L : 300ns ~ 100us
	RES : 300 us or up

	8MHz = bit clok 125 ns
	T1H 900
	T1L 350

	T0H 350
	T0L 900

*/

// #define WS_SPI_FREQ 								(7000000ul)
// #define WS_SPI_HIGH 								(0xFC)
// #define WS_SPI_LOW	 								(0xC0)

#define WS_SPI_FREQ 								(8000000ul)
#define WS_SPI_HIGH 								(0xF8)
#define WS_SPI_LOW	 								(0xE0)

#define WS_RES_POS_FRONT							(0) 
#define WS_RES_POS_BACK 							(1) 


#define MS_LED_LATCH								(2000ul)


extern uint8_t DemoState;

extern uint8_t ramBuffer2[LED_LLSI_DATA_LEN];
extern uint8_t ramBuffer3[LED_LLSI_DATA_LEN];
extern uint8_t ramBuffer4[LED_LLSI_DATA_LEN];
extern uint8_t ramBuffer5[LED_LLSI_DATA_LEN];
extern uint8_t SpiBuffer[LED_SPI_DATA_LEN*8];

/*_____ M A C R O S ________________________________________________________*/

/*_____ F U N C T I O N S __________________________________________________*/


void setLED_Display(uint8_t idx);
void setLED_BufferClear(uint8_t idx);
void setLED_ColorArrange(uint8_t idx,uint16_t DeviceNumber ,uint8_t RED, uint8_t GREEN, uint8_t BLUE);
void setLED_ColorWipe(uint8_t idx , uint8_t RED, uint8_t GREEN, uint8_t BLUE);
// void setLED_ColorIdxChange(void);
void _stripEffect_Rainbow(uint8_t* RED, uint8_t* GREEN, uint8_t* BLUE);	
uint8_t _stripEffect_Wheel(uint8_t WheelPos , uint8_t* RED, uint8_t* GREEN, uint8_t* BLUE); 

void getFromAToB(uint16_t time, uint16_t steps, 
		uint8_t redA,	uint8_t greenA, uint8_t blueA, 
		uint8_t redB, uint8_t greenB, uint8_t blueB); 
void getCircularRing(uint16_t time, uint8_t red, uint8_t green,uint8_t blue); 
void getHeartBeat(uint16_t time, uint8_t red, uint8_t green,uint8_t blue); 

// #if defined (ENABLE_EXTRA_DEMO)
void getAllColors(uint16_t time);
void getColorWheel(uint16_t time); 
// #endif /*ENABLE_EXTRA_DEMO*/

void getPatternMove(uint16_t time, uint16_t parts, uint8_t red,uint8_t green, uint8_t blue); 
void getFullEmpty(uint16_t time, uint8_t red, uint8_t green,uint8_t blue); 
void getAlternateColors(uint16_t time, uint16_t steps,
		uint8_t redA, uint8_t greenA, uint8_t blueA, 
		uint8_t redB, uint8_t greenB, uint8_t blueB); 

void getRGBSequence(uint16_t time); 
void getTheaterChaseRainbow(uint16_t time); 
void getTheaterChase(uint16_t time ,uint8_t RED, uint8_t GREEN, uint8_t BLUE); 
void getRainbowCycle(uint8_t cycle , uint16_t time); 
void getRainbow(uint16_t time);
void getBreathing(uint16_t time);
void getSimpleTest(void);
void StateMachine(void);

void led_effect(void);
void dispayString_Statemachine(void);


/*** (C) COPYRIGHT 2017 Nuvoton Technology Corp. ***/
