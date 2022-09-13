# NUC1262BSP_LLSI_SPI_RGBLED
 NUC1262BSP_LLSI_SPI_RGBLED


update @ 2022/09/13

1. use SPI0 / SPI / LLSI0~3 , to drive LED stripe

	SPI0 port:	
	MOSI	:PA0	
	CLK		:PA2	
	SS		:PA3

	SPI1 port:	
	MOSI	:PB4	
	CLK		:PB3	
	SS		:PB2	

	LLSI port:	
	LLSI0	:PC5	
	LLSI1	:PC4	
	LLSI2	:PC3	
	LLSI3	:PC2

2. by using terminal to control LED pattern , press ? , to see available pattern

	(1)state_Rainbow
	(2)state_RainbowCycle
	(3)state_TheaterChase
	(4)state_TheaterChaseRainbow
	(5)state_RGBSequence
	(6)state_AlternateColors
	(7)state_FullEmpty
	(8)state_PatternMove
	(9)state_ColorWheel
	(:)state_AllColors
	(;)state_HeartBeat
	(<)state_CircularRing
	(=)state_FromAToB

	(e)wipe pattern
	(r)R pattern	
	(g)G pattern
	(b)B pattern	

![image](https://github.com/released/NUC1262BSP_LLSI_SPI_RGBLED/blob/main/log.jpg)

3. below is LA capture , 

SPI0

![image](https://github.com/released/NUC1262BSP_LLSI_SPI_RGBLED/blob/main/LA_SPI0.jpg)

SPI1

![image](https://github.com/released/NUC1262BSP_LLSI_SPI_RGBLED/blob/main/LA_SPI1.jpg)

LLSI 0 ~ 3

![image](https://github.com/released/NUC1262BSP_LLSI_SPI_RGBLED/blob/main/LA_LLSI.jpg)



