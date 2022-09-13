/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

#include "project_config.h"
#include "led_effect.h"
/*_____ D E C L A R A T I O N S ____________________________________________*/

/*_____ D E F I N I T I O N S ______________________________________________*/
volatile uint32_t BitFlag = 0;
volatile uint32_t counter_tick = 0;

#define CRYSTAL_LESS                        (1)
// #define SYS_CLOCK                           (72000000ul)
#define SYS_CLOCK                           (48000000ul)

uint8_t LLSI0_Transfer_flag = 0xFF;
uint8_t LLSI1_Transfer_flag = 0xFF;
uint8_t LLSI2_Transfer_flag = 0xFF;
uint8_t LLSI3_Transfer_flag = 0xFF;

/*_____ M A C R O S ________________________________________________________*/

/*_____ F U N C T I O N S __________________________________________________*/


void tick_counter(void)
{
	counter_tick++;
}

uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void compare_buffer(uint8_t *src, uint8_t *des, int nBytes)
{
    uint16_t i = 0;	
	
    for (i = 0; i < nBytes; i++)
    {
        if (src[i] != des[i])
        {
            printf("error idx : %4d : 0x%2X , 0x%2X\r\n", i , src[i],des[i]);
			set_flag(flag_error , ENABLE);
        }
    }

	if (!is_flag_set(flag_error))
	{
    	printf("%s finish \r\n" , __FUNCTION__);	
		set_flag(flag_error , DISABLE);
	}

}

void reset_buffer(void *dest, unsigned int val, unsigned int size)
{
    uint8_t *pu8Dest;
//    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;

	#if 1
	while (size-- > 0)
		*pu8Dest++ = val;
	#else
	memset(pu8Dest, val, size * (sizeof(pu8Dest[0]) ));
	#endif
	
}

void copy_buffer(void *dest, void *src, unsigned int size)
{
    uint8_t *pu8Src, *pu8Dest;
    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;
    pu8Src  = (uint8_t *)src;


	#if 0
	  while (size--)
	    *pu8Dest++ = *pu8Src++;
	#else
    for (i = 0; i < size; i++)
        pu8Dest[i] = pu8Src[i];
	#endif
}

void dump_buffer(uint32_t *pucBuff, int nBytes)
{
    uint16_t i = 0;
    
    printf("dump_buffer : %2d\r\n" , nBytes);    
    for (i = 0 ; i < nBytes ; i++)
    {
        printf("0x%08X," , pucBuff[i]);
        if ((i+1)%8 ==0)
        {
            printf("\r\n");
        }            
    }
    printf("\r\n\r\n");
}

void  dump_buffer_hex(uint8_t *pucBuff, int nBytes)
{
    int     nIdx, i;

    nIdx = 0;
    while (nBytes > 0)
    {
        printf("0x%04X  ", nIdx);
        for (i = 0; i < 16; i++)
            printf("%02X ", pucBuff[nIdx + i]);
        printf("  ");
        for (i = 0; i < 16; i++)
        {
            if ((pucBuff[nIdx + i] >= 0x20) && (pucBuff[nIdx + i] < 127))
                printf("%c", pucBuff[nIdx + i]);
            else
                printf(".");
            nBytes--;
        }
        nIdx += 16;
        printf("\n");
    }
    printf("\n");
}

void delay(uint16_t dly)
{
/*
	delay(100) : 14.84 us
	delay(200) : 29.37 us
	delay(300) : 43.97 us
	delay(400) : 58.5 us	
	delay(500) : 73.13 us	
	
	delay(1500) : 0.218 ms (218 us)
	delay(2000) : 0.291 ms (291 us)	
*/

	while( dly--);
}


void delay_ms(uint16_t ms)
{
	TIMER_Delay(TIMER0, 1000*ms);
}

void SPI_PDMA_Start(SPI_T *spi , uint8_t* InBuffer ,uint32_t len)
{
    uint32_t u32Ch = 0;
    uint32_t u32Mask = 0;    
    uint32_t u32Peripheral = 0;

	uint32_t u32RegValue = 0;
	uint32_t u32Abort = 0;	
    uint8_t Err = 0xFF;

    if (spi == LED_PORT0)
    {
	    u32Ch = LED_PORT0_PDMA_CH;
        u32Peripheral = PDMA_SPI0_TX;
    }
    else if (spi == LED_PORT1)
    {
	    u32Ch = LED_PORT1_PDMA_CH;
        u32Peripheral = PDMA_SPI1_TX;        
    }

    u32Mask = 1 << u32Ch;

    SPI_ClearTxFIFO(spi);

    /* Reset PDMA module */
    SYS_ResetModule(PDMA_RST);

    /* Open Channel 0 */
    PDMA_Open(u32Mask);    

	//TX
	PDMA_SetTransferCnt(u32Ch, PDMA_WIDTH_8, len);
	PDMA_SetTransferAddr(u32Ch, (uint32_t)InBuffer, PDMA_SAR_INC, (uint32_t)&spi->TX, PDMA_DAR_FIX);		
	/* Set request source; set basic mode. */
	PDMA_SetTransferMode(u32Ch, u32Peripheral, FALSE, 0);
    //Select Single Request
    PDMA_SetBurstType(u32Ch, PDMA_REQ_SINGLE, 0);
    /* Disable table interrupt */
    // PDMA->DSCT[SPI_MASTER_TX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;	
	PDMA_DisableInt(u32Ch,PDMA_INT_TEMPTY);
		
	SPI_TRIGGER_TX_PDMA(spi);

	#if 0 //debug
	printf("SPI TX start (ch:0x%2X)" ,u32Ch);	
	#endif

    // wait PDMA done
	while(1)
    {
         /* Get interrupt status */
        u32RegValue = PDMA_GET_INT_STATUS();
        /* Check the DMA transfer done interrupt flag */
        if(u32RegValue & PDMA_INTSTS_TDIF_Msk)
        {
            /* Check the PDMA transfer done interrupt flags */
            if((PDMA_GET_TD_STS() & (u32Mask)) == (u32Mask))
            {
                /* Clear the DMA transfer done flags */
                PDMA_CLR_TD_FLAG(u32Mask);
                /* Disable SPI PDMA TX function */
                SPI_DISABLE_TX_PDMA(spi);
                Err = 0;
                break;
            }
        }    

        /* Check the DMA transfer abort interrupt flag */
        if(u32RegValue & PDMA_INTSTS_ABTIF_Msk)
        {
            /* Get the target abort flag */
            u32Abort = PDMA_GET_ABORT_STS();
            /* Clear the target abort flag */
            PDMA_CLR_ABORT_FLAG(u32Abort);
            Err = 1;
            break;
        }        

    }

	while (SPI_IS_BUSY(spi));	

	#if 0 //debug
	printf("--done(0x%2X)\r\n" , Err);	
	#endif    
}


void LLSI_PDMA_Start(LLSI_T *llsi , uint32_t len)
{
    uint32_t u32Ch = 0;
    // uint32_t u32Mask = 0;   
    uint32_t u32Peripheral = 0;

	uint32_t u32RegValue = 0;
	uint32_t u32Abort = 0;	

    if (llsi == LED_PORT2)
    {
        u32Ch = LED_PORT2_PDMA_CH;
        u32Peripheral = PDMA_LLSI0;  
        LLSI0_Transfer_flag = 0;
    }
    else if (llsi == LED_PORT3)
    {
        u32Ch = LED_PORT3_PDMA_CH;
        u32Peripheral = PDMA_LLSI1;    
        LLSI1_Transfer_flag = 0;  
    }
    else if (llsi == LED_PORT4)
    {
        u32Ch = LED_PORT4_PDMA_CH;
        u32Peripheral = PDMA_LLSI2;   
        LLSI2_Transfer_flag = 0;  
    }
    else if (llsi == LED_PORT5)
    {
        u32Ch = LED_PORT5_PDMA_CH;
        u32Peripheral = PDMA_LLSI3;   
        LLSI3_Transfer_flag = 0;  
    }

    // u32Mask = 1 << u32Ch; 

    /* Transfer count is TEST_COUNT, transfer width is 32 bits(one word) */
    PDMA_SetTransferCnt(u32Ch, PDMA_WIDTH_32, len);
    /* Request source is LLSI3 */
    PDMA_SetTransferMode(u32Ch, u32Peripheral, FALSE, 0);

	#if 0 //debug
	printf("LLSI start (ch:0x%2X)\r\n" , u32Ch);	
	#endif

    // LLSI_SET_PDMA_MODE((LLSI_T *)(APB1_BASE + 0x100000*(LED_Setting->LLSI_Num%2) + 0x54000 + 0x200*(LED_Setting->LLSI_Num/2)));
    LLSI_SET_PDMA_MODE(llsi);

    #if 1
    if (llsi == LED_PORT2)
    {
        while(!LLSI0_Transfer_flag);
    }
    else if (llsi == LED_PORT3)
    {
        while(!LLSI1_Transfer_flag);
    }
    else if (llsi == LED_PORT4)
    {
        while(!LLSI2_Transfer_flag);
    }
    else if (llsi == LED_PORT5)
    {
        while(!LLSI3_Transfer_flag);
    }
    #endif

	#if 0 //debug
	printf("--done\r\n");	
	#endif        
}

void LLSI3_IRQHandler(void)
{
    LLSI_T *llsi = LED_PORT5;    
    if(LLSI_GetIntFlag(llsi, LLSI_RST_INT_MASK))
    {
        LLSI_ClearIntFlag(llsi, LLSI_RST_INT_MASK); 
        // printf("LLSI3 RST\r\n"); 

        if (!LLSI3_Transfer_flag)   
        {
            LLSI3_Transfer_flag = 1; 
            LLSI_SET_SW_MODE(llsi);
        }       
    }
    if(LLSI_GetIntFlag(llsi, LLSI_FEND_INT_MASK))
    {
        LLSI_ClearIntFlag(llsi, LLSI_FEND_INT_MASK); 
        // printf("LLSI3 FRAME END\r\n");                       
    }    
}

void LLSI2_IRQHandler(void)
{
    LLSI_T *llsi = LED_PORT4;    
    if(LLSI_GetIntFlag(llsi, LLSI_RST_INT_MASK))
    {
        LLSI_ClearIntFlag(llsi, LLSI_RST_INT_MASK); 
        // printf("LLSI2 RST\r\n");

        if (!LLSI2_Transfer_flag)   
        {
            LLSI2_Transfer_flag = 1;
            LLSI_SET_SW_MODE(llsi);
        }        
    }
    if(LLSI_GetIntFlag(llsi, LLSI_FEND_INT_MASK))
    {
        LLSI_ClearIntFlag(llsi, LLSI_FEND_INT_MASK); 
        // printf("LLSI2 FRAME END\r\n");                        
    }    
}

void LLSI1_IRQHandler(void)
{
    LLSI_T *llsi = LED_PORT3;    
    if(LLSI_GetIntFlag(llsi, LLSI_RST_INT_MASK))
    {
        LLSI_ClearIntFlag(llsi, LLSI_RST_INT_MASK); 
        // printf("LLSI1 RST\r\n");

        if (!LLSI1_Transfer_flag)   
        {
            LLSI1_Transfer_flag = 1; 
            LLSI_SET_SW_MODE(llsi);
        }        
    }
    if(LLSI_GetIntFlag(llsi, LLSI_FEND_INT_MASK))
    {
        LLSI_ClearIntFlag(llsi, LLSI_FEND_INT_MASK); 
        // printf("LLSI1 FRAME END\r\n");                       
    }    
}

void LLSI0_IRQHandler(void)
{
    LLSI_T *llsi = LED_PORT2;
    if(LLSI_GetIntFlag(llsi, LLSI_RST_INT_MASK))
    {
        LLSI_ClearIntFlag(llsi, LLSI_RST_INT_MASK); 
        // printf("LLSI0 RST\r\n");    

        if (!LLSI0_Transfer_flag)   
        {
            LLSI0_Transfer_flag = 1; 
            LLSI_SET_SW_MODE(llsi);
        }
    }
    if(LLSI_GetIntFlag(llsi, LLSI_FEND_INT_MASK))
    {
        LLSI_ClearIntFlag(llsi, LLSI_FEND_INT_MASK); 
        // printf("LLSI0 FRAME END\r\n");                
    }    
}

void LLSI_Init(void)
{
	/*
		WS2812 : TH+TL=1.25us}600ns , GRB
		T0H : 220ns ~ 380ns
		T1H : 580ns ~ 1.6us
		T0L : 580ns ~ 1.6us
		T1L : 220ns ~ 420ns
		RES : 280 us or up

		WS2813 : TH+TL=1.25us}300ns , RGB
		T0H : 300ns ~ 450ns
		T1H : 750ns ~ 1000ns
		T0L : 300ns ~ 100us
		T1L : 300ns ~ 100us
		RES : 300 us or up

	*/
	const uint32_t u32BusClock = SYS_CLOCK;
	#if 0   // WS2812
	const uint32_t u32TransferTimeNsec = 1125;
	const uint32_t u32T0HTimeNsec = 375;
	const uint32_t u32T1HTimeNsec = 750;
	#else   // WS2813
	const uint32_t u32TransferTimeNsec = 1250;
	const uint32_t u32T0HTimeNsec = 350;
	const uint32_t u32T1HTimeNsec = 900;

	#endif
	const uint32_t u32ResetTimeNsec = 50000;
    const uint32_t u32LLSIMode = LLSI_MODE_SW;  //LLSI_MODE_PDMA;
    const uint32_t u32OutputFormat = LLSI_FORMAT_RGB;    

    LLSI_Open(LED_PORT2, 
			u32LLSIMode, 
			u32OutputFormat, 
			u32BusClock, 
			u32TransferTimeNsec, 
			u32T0HTimeNsec, 
			u32T1HTimeNsec, 
			u32ResetTimeNsec, 
			LED_NUM , 
			LLSI_IDLE_LOW);
    LLSI_Open(LED_PORT3, 
			u32LLSIMode, 
			u32OutputFormat, 
			u32BusClock, 
			u32TransferTimeNsec, 
			u32T0HTimeNsec, 
			u32T1HTimeNsec, 
			u32ResetTimeNsec, 
			LED_NUM , 
			LLSI_IDLE_LOW);
    LLSI_Open(LED_PORT4, 
			u32LLSIMode, 
			u32OutputFormat, 
			u32BusClock, 
			u32TransferTimeNsec, 
			u32T0HTimeNsec, 
			u32T1HTimeNsec, 
			u32ResetTimeNsec, 
			LED_NUM , 
			LLSI_IDLE_LOW);
    LLSI_Open(LED_PORT5, 
			u32LLSIMode, 
			u32OutputFormat,  
			u32BusClock, 
			u32TransferTimeNsec, 
			u32T0HTimeNsec, 
			u32T1HTimeNsec, 
			u32ResetTimeNsec, 
			LED_NUM , 
			LLSI_IDLE_LOW);

    /* Enable reset command function */
    LLSI_ENABLE_RESET_COMMAND(LED_PORT2);
    LLSI_ENABLE_RESET_COMMAND(LED_PORT3);
    LLSI_ENABLE_RESET_COMMAND(LED_PORT4);
    LLSI_ENABLE_RESET_COMMAND(LED_PORT5);

    LLSI_EnableInt(LED_PORT2, LLSI_RST_INT_MASK | LLSI_FEND_INT_MASK );
    LLSI_EnableInt(LED_PORT3, LLSI_RST_INT_MASK | LLSI_FEND_INT_MASK );
    LLSI_EnableInt(LED_PORT4, LLSI_RST_INT_MASK | LLSI_FEND_INT_MASK );
    LLSI_EnableInt(LED_PORT5, LLSI_RST_INT_MASK | LLSI_FEND_INT_MASK );
    
    NVIC_EnableIRQ(LLSI0_IRQn);
    NVIC_EnableIRQ(LLSI1_IRQn);
    NVIC_EnableIRQ(LLSI2_IRQn);
    NVIC_EnableIRQ(LLSI3_IRQn);

    /* Reset PDMA module */
    // NVIC_DisableIRQ(PDMA_IRQn);
    SYS_ResetModule(PDMA_RST);
    
    /* Open Channel 0 */
    PDMA_Open(LED_PORT2_PDMA_OPENED_CH | LED_PORT3_PDMA_OPENED_CH | LED_PORT4_PDMA_OPENED_CH | LED_PORT5_PDMA_OPENED_CH);   

    /* Transfer type is single transfer */
    PDMA_SetBurstType(LED_PORT2_PDMA_CH , PDMA_REQ_SINGLE, 0);
    PDMA_SetBurstType(LED_PORT3_PDMA_CH , PDMA_REQ_SINGLE, 0);
    PDMA_SetBurstType(LED_PORT4_PDMA_CH , PDMA_REQ_SINGLE, 0);
    PDMA_SetBurstType(LED_PORT5_PDMA_CH , PDMA_REQ_SINGLE, 0);

    PDMA_SetTransferAddr(LED_PORT2_PDMA_CH, (uint32_t)ramBuffer2, PDMA_SAR_INC, (uint32_t)&LED_PORT2->DATA, PDMA_DAR_FIX); 
    PDMA_SetTransferAddr(LED_PORT3_PDMA_CH, (uint32_t)ramBuffer3, PDMA_SAR_INC, (uint32_t)&LED_PORT3->DATA, PDMA_DAR_FIX);  
    PDMA_SetTransferAddr(LED_PORT4_PDMA_CH, (uint32_t)ramBuffer4, PDMA_SAR_INC, (uint32_t)&LED_PORT4->DATA, PDMA_DAR_FIX);  
    PDMA_SetTransferAddr(LED_PORT5_PDMA_CH, (uint32_t)ramBuffer5, PDMA_SAR_INC, (uint32_t)&LED_PORT5->DATA, PDMA_DAR_FIX);     

}

void SPI_Init(void)
{
	SPI_Open(LED_PORT0, SPI_MASTER, SPI_MODE_0, 8, WS_SPI_FREQ);
	SPI_EnableAutoSS(LED_PORT0, SPI_SS, SPI_SS_ACTIVE_LOW);    

	SPI_Open(LED_PORT1, SPI_MASTER, SPI_MODE_0, 8, WS_SPI_FREQ);
	SPI_EnableAutoSS(LED_PORT1, SPI_SS, SPI_SS_ACTIVE_LOW);   

}

void loop(void)
{
//	static uint32_t LOG = 0;

	led_effect();

    if (is_flag_set(flag_timer_period_1000ms))
    {
        set_flag(flag_timer_period_1000ms ,DISABLE);

        // printf("%s(timer) : %4d\r\n",__FUNCTION__,LOG++);
        PB14 ^= 1;        
    }    

    if (is_flag_set(flag_pattern_R))
    {        
        setLED_ColorWipe(LED_STRIPE_IDX_0 ,0xFF , 0x00 , 0x00);
        setLED_ColorWipe(LED_STRIPE_IDX_1 ,0xFF , 0x00 , 0x00);
        setLED_ColorWipe(LED_STRIPE_IDX_2 ,0xFF , 0x00 , 0x00);	
        setLED_ColorWipe(LED_STRIPE_IDX_3 ,0xFF , 0x00 , 0x00);	
        setLED_ColorWipe(LED_STRIPE_IDX_4 ,0xFF , 0x00 , 0x00);	
        setLED_ColorWipe(LED_STRIPE_IDX_5 ,0xFF , 0x00 , 0x00);	        
    }

    if (is_flag_set(flag_pattern_G))
    {        
        setLED_ColorWipe(LED_STRIPE_IDX_0 ,0x00 , 0xFF , 0x00);
        setLED_ColorWipe(LED_STRIPE_IDX_1 ,0x00 , 0xFF , 0x00);
        setLED_ColorWipe(LED_STRIPE_IDX_2 ,0x00 , 0xFF , 0x00);	
        setLED_ColorWipe(LED_STRIPE_IDX_3 ,0x00 , 0xFF , 0x00);	
        setLED_ColorWipe(LED_STRIPE_IDX_4 ,0x00 , 0xFF , 0x00);	
        setLED_ColorWipe(LED_STRIPE_IDX_5 ,0x00 , 0xFF , 0x00);	        
    }

    if (is_flag_set(flag_pattern_B))
    {     
        setLED_ColorWipe(LED_STRIPE_IDX_0 ,0x00 , 0x00 , 0xFF);
        setLED_ColorWipe(LED_STRIPE_IDX_1 ,0x00 , 0x00 , 0xFF);
        setLED_ColorWipe(LED_STRIPE_IDX_2 ,0x00 , 0x00 , 0xFF);	
        setLED_ColorWipe(LED_STRIPE_IDX_3 ,0x00 , 0x00 , 0xFF);	
        setLED_ColorWipe(LED_STRIPE_IDX_4 ,0x00 , 0x00 , 0xFF);	
        setLED_ColorWipe(LED_STRIPE_IDX_5 ,0x00 , 0x00 , 0xFF);	        
    }    

    if (is_flag_set(flag_pattern_wipe))
    {
        set_flag(flag_pattern_wipe,DISABLE);
        setLED_ColorWipe(LED_STRIPE_IDX_0 ,0x00 , 0x00 , 0x00);
        setLED_ColorWipe(LED_STRIPE_IDX_1 ,0x00 , 0x00 , 0x00);
        setLED_ColorWipe(LED_STRIPE_IDX_2 ,0x00 , 0x00 , 0x00);	
        setLED_ColorWipe(LED_STRIPE_IDX_3 ,0x00 , 0x00 , 0x00);	
        setLED_ColorWipe(LED_STRIPE_IDX_4 ,0x00 , 0x00 , 0x00);	
        setLED_ColorWipe(LED_STRIPE_IDX_5 ,0x00 , 0x00 , 0x00);	

    }

}


void GPIO_Init (void)
{
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB14MFP_Msk)) | (SYS_GPB_MFPH_PB14MFP_GPIO);
//    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB15MFP_Msk)) | (SYS_GPB_MFPH_PB15MFP_GPIO);
	
    GPIO_SetMode(PB, BIT14, GPIO_MODE_OUTPUT);
//    GPIO_SetMode(PB, BIT15, GPIO_MODE_OUTPUT);	
}


void TMR1_IRQHandler(void)
{	
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();

		if ((get_tick() % 1000) == 0)
		{
            set_flag(flag_timer_period_1000ms ,ENABLE);
		}

		if ((get_tick() % 50) == 0)
		{

		}	
    }
}


void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

void UARTx_Process(void)
{
	uint8_t res = 0;
	res = UART_READ(UART0);
    printf("pressed : %c(0x%2X)\r\n" , res, res);

	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{
        set_flag(flag_pattern_R,DISABLE);
        set_flag(flag_pattern_G,DISABLE);
        set_flag(flag_pattern_B,DISABLE);     
        set_flag(flag_pattern_wipe,DISABLE); 

		DemoState = res - 0x30;
		printf("DemoState(%c) = %2d(0x%2X)\r\n" , res , DemoState , DemoState);

		switch(res)
		{       

			case 'r':
			case 'R':
                set_flag(flag_pattern_R,ENABLE);            
				break;

			case 'g':
			case 'G':
                set_flag(flag_pattern_G,ENABLE);            
				break;

			case 'b':
			case 'B':
                set_flag(flag_pattern_B,ENABLE);            
				break;         

            case 'e':
            case 'E':
                set_flag(flag_pattern_wipe ,ENABLE);
                break;

            case '?':
                dispayString_Statemachine();
                break;

			case 'X':
			case 'x':
			case 'Z':
			case 'z':
				NVIC_SystemReset();		
				break;
		}
	}
}

void UART0_IRQHandler(void)
{

    if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RDAINT_Msk | UART_INTSTS_RXTOINT_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
            UARTx_Process();
        }
    }

    if(UART0->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
    {
        UART_ClearIntFlag(UART0, (UART_INTSTS_RLSINT_Msk| UART_INTSTS_BUFERRINT_Msk));
    }	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);
    UART_EnableInt(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_RXTOIEN_Msk);
    NVIC_EnableIRQ(UART0_IRQn);
	
	#if (_debug_log_UART_ == 1)	//debug
	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());	
	printf("CLK_GetHCLKFreq : %8d\r\n",CLK_GetHCLKFreq());		
	#endif	

}

void SYS_Init(void)
{
    SYS_UnlockReg();	

    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    /* Set PF multi-function pins for XT1_OUT(PF.2) and XT1_IN(PF.3) */
//    SYS->GPF_MFPL = (SYS->GPF_MFPL & (~SYS_GPF_MFPL_PF2MFP_Msk)) | SYS_GPF_MFPL_PF2MFP_XT1_OUT;
//    SYS->GPF_MFPL = (SYS->GPF_MFPL & (~SYS_GPF_MFPL_PF3MFP_Msk)) | SYS_GPF_MFPL_PF3MFP_XT1_IN;

    /* Enable GPIO Port F module clock */
//    CLK_EnableModuleClock(GPIOF_MODULE);

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) as input mode to use HXT */
//    GPIO_SetMode(PF, BIT2|BIT3, GPIO_MODE_INPUT);

//    CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);	

//    CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);		
	

#if (!CRYSTAL_LESS)
    /* Enable GPIO Port F module clock */
    CLK_EnableModuleClock(GPIOF_MODULE);

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) as input mode to use HXT */
    GPIO_SetMode(PF, BIT2|BIT3, GPIO_MODE_INPUT);

    /* Enable HXT clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);

    /* Wait for HXT clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    /* Set core clock to 72MHz */
    CLK_SetCoreClock(72000000);

    /* Select USB clock source as PLL and USB clock divider as 3 */
    CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL3_USBDSEL_PLL, CLK_CLKDIV0_USB(3));
#else
    /* Select HCLK clock source to HIRC and HCLK clock divider as 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    /* Select USB clock source as HIRC and USB clock divider as 1 */
    CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL3_USBDSEL_HIRC, CLK_CLKDIV0_USB(1));
#endif

    CLK_EnableModuleClock(UART0_MODULE);
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC_DIV2, CLK_CLKDIV0_UART0(1));

    CLK_EnableModuleClock(TMR0_MODULE);
  	CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HIRC_DIV2, 0);

    CLK_EnableModuleClock(TMR1_MODULE);
  	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC_DIV2, 0);

    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_PCLK0, MODULE_NoMsk);
    CLK_EnableModuleClock(SPI0_MODULE);

    CLK_SetModuleClock(SPI1_MODULE, CLK_CLKSEL2_SPI1SEL_PCLK0, MODULE_NoMsk);
    CLK_EnableModuleClock(SPI1_MODULE);    

    SYS_ResetModule(LLSI0_RST);
    SYS_ResetModule(LLSI1_RST);
    SYS_ResetModule(LLSI2_RST);
    SYS_ResetModule(LLSI3_RST);

    /* Enable LLSI module clock */
    CLK_EnableModuleClock(LLSI0_MODULE);
    CLK_EnableModuleClock(LLSI1_MODULE);
    CLK_EnableModuleClock(LLSI2_MODULE);
    CLK_EnableModuleClock(LLSI3_MODULE);
	
    /* Enable PDMA peripheral clock */
    CLK_EnableModuleClock(PDMA_MODULE);

    /* Set PB multi-function pins for UART0 RXD=PB.12 and TXD=PB.13 */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk)) |
                    (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

//    SYS->GPB_MFPH = (SYS->GPB_MFPH & (~(UART0_RXD_PB12_Msk | UART0_TXD_PB13_Msk))) | (UART0_RXD_PB12 | UART0_TXD_PB13);

    /*
        LLSI0	:PC5
        LLSI1	:PC4
        LLSI2	:PC3
        LLSI3	:PC2
    */
    // SYS->GPC_MFPL = (SYS->GPC_MFPL & ~(SYS_GPC_MFPL_PC5MFP_Msk)) | SYS_GPC_MFPL_PC5MFP_LLSI0_OUT;
    // SYS->GPC_MFPL = (SYS->GPC_MFPL & ~(SYS_GPC_MFPL_PC4MFP_Msk)) | SYS_GPC_MFPL_PC4MFP_LLSI1_OUT;
    // SYS->GPC_MFPL = (SYS->GPC_MFPL & ~(SYS_GPC_MFPL_PC3MFP_Msk)) | SYS_GPC_MFPL_PC3MFP_LLSI2_OUT;
    // SYS->GPC_MFPL = (SYS->GPC_MFPL & ~(SYS_GPC_MFPL_PC2MFP_Msk)) | SYS_GPC_MFPL_PC2MFP_LLSI3_OUT;
    SET_LLSI0_OUT_PC5();
    SET_LLSI1_OUT_PC4();
    SET_LLSI2_OUT_PC3();
    SET_LLSI3_OUT_PC2();

    /*
        SPI0 port:
        MOSI	:PA0
        CLK		:PA2
        SS		:PA3

        SPI1 port:
        MOSI	:PB4
        CLK		:PB3
        SS		:PB2	
    */
    SYS->GPA_MFPL = (SYS->GPA_MFPL & ~(SYS_GPA_MFPL_PA0MFP_Msk)) | SYS_GPA_MFPL_PA0MFP_SPI0_MOSI;
    SYS->GPA_MFPL = (SYS->GPA_MFPL & ~(SYS_GPA_MFPL_PA2MFP_Msk)) | SYS_GPA_MFPL_PA2MFP_SPI0_CLK;
    SYS->GPA_MFPL = (SYS->GPA_MFPL & ~(SYS_GPA_MFPL_PA3MFP_Msk)) | SYS_GPA_MFPL_PA3MFP_SPI0_SS;   

    SYS->GPB_MFPL = (SYS->GPB_MFPL & ~(SYS_GPB_MFPL_PB4MFP_Msk)) | SYS_GPB_MFPL_PB4MFP_SPI1_MOSI;
    SYS->GPB_MFPL = (SYS->GPB_MFPL & ~(SYS_GPB_MFPL_PB3MFP_Msk)) | SYS_GPB_MFPL_PB3MFP_SPI1_CLK;
    SYS->GPB_MFPL = (SYS->GPB_MFPL & ~(SYS_GPB_MFPL_PB2MFP_Msk)) | SYS_GPB_MFPL_PB2MFP_SPI1_SS; 

   /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();
}

int main()
{
    SYS_Init();

	UART0_Init();
	GPIO_Init();
	TIMER1_Init();

    SPI_Init();

	LLSI_Init();   

	// need to put some data into FIFO first
	setLED_ColorWipe(LED_STRIPE_IDX_0 ,0x00 , 0x00 , 0x00);
	setLED_ColorWipe(LED_STRIPE_IDX_1 ,0x00 , 0x00 , 0x00);
	setLED_ColorWipe(LED_STRIPE_IDX_2 ,0x00 , 0x00 , 0x00);	
	setLED_ColorWipe(LED_STRIPE_IDX_3 ,0x00 , 0x00 , 0x00);	
	setLED_ColorWipe(LED_STRIPE_IDX_4 ,0x00 , 0x00 , 0x00);	
	setLED_ColorWipe(LED_STRIPE_IDX_5 ,0x00 , 0x00 , 0x00);	     

    /* Got no where to go, just loop forever */
    while(1)
    {
        loop();

    }
}

/*** (C) COPYRIGHT 2021 Nuvoton Technology Corp. ***/
