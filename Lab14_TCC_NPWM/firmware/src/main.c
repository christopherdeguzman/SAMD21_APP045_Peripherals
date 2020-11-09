/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes
#include <stdio.h>
#include <string.h>

//TODO 17.01
#include "Alphabet_Fonts.h"
#include "Microchip_Logo.h"

// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************
uint32_t i = 0;

volatile uint8_t TC3_IsOverflow = 0;
volatile uint8_t TC4_IsOverflow = 0;
uint8_t USARTRTxBuffer[100];
uint8_t USART5_ReceiveData[1];
volatile uint8_t USART5_IsReceived = 0;
volatile uint8_t USART5_IsTransmitted = 1;
uint16_t ADC_Result[2];
volatile uint8_t ADC_IsCompleted = 0;
volatile int8_t ADC_ChannelIdx = 0;

//TODO 16.01
uint8_t Duty = 50;
int8_t DutyDistance = 2;

//TODO 17.02
#define LCM_WIDTH   128
#define LCM_HEIGHT  64
#define FONT_WIDTH  7
#define FONT_HEIGHT 8
uint8_t LogoX, LogoY;
unsigned char LCM_Blank[1024];

char Alphabet[95] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz~!@#$%^&()[]{}_\\|;:,.`'\"<>+-*/=? ";

const uint8_t LCM_InitCMD[] = {
    0xAE,          // DISPLAY OFF
	0xD5,          // SET OSC FREQUENY
	0x80,          // divide ratio = 1 (bit 3-0), OSC (bit 7-4)
	0xA8,          // SET MUX RATIO
	0x3F,          // 64MUX
	0xD3,          // SET DISPLAY OFFSET
	0x00,          // offset = 0
	0x40,          // set display start line, start line = 0
	0x8D,          // ENABLE CHARGE PUMP REGULATOR
	0x14,          //
	0x20,          // SET MEMORY ADDRESSING MODE
	0x02,          // horizontal addressing mode
	0xA1,          // set segment re-map, column address 127 is mapped to SEG0
	0xC8,          // set COM/Output scan direction, remapped mode (COM[N-1] to COM0)
	0xDA,          // SET COM PINS HARDWARE CONFIGURATION
	0x12,          // alternative COM pin configuration
	0x81,          // SET CONTRAST CONTROL
	0xCF,          //
	0xD9,          // SET PRE CHARGE PERIOD
	0xF1,          //
	0xDB,          // SET V_COMH DESELECT LEVEL
	0x40,          //
	0xA4,          // DISABLE ENTIRE DISPLAY ON
	0xA6,          // NORMAL MODE (A7 for inverse display)
	0xAF	       // DISPLAY ON
};

void LCM_SetCursor (uint8_t x, uint8_t y) {
    uint8_t OUT[3] = {0x0F & x, 0x10 +(x >> 4), 0xB0 + y};
    SSD1306_RS_PIN_Clear();
    SERCOM4_SPI_Write((void*)OUT, 3);
}

void LCM_DrawBitmap( uint8_t sx, uint8_t sy, uint8_t width, uint8_t height, const unsigned char *byte )
{
	for( int y = 0; y < (height/8); y++ )
	{
		LCM_SetCursor(sx, sy + y);
		SSD1306_RS_PIN_Set(); // Data
		SERCOM4_SPI_Write(( void* )byte, (width - sx));
		byte += width;
	}
}

void LCM_DrawFont( uint8_t sx, uint8_t row, char ch )
{
	int chIdx = 0;

	// Index character
	while( ch != Alphabet[chIdx] ) { chIdx++; }
	
	LCM_SetCursor(sx, row);
	SSD1306_RS_PIN_Set(); // Data
	SERCOM4_SPI_Write( ( void* )(Alphabet_Fonts+(chIdx*FONT_WIDTH)), FONT_WIDTH );
}

void LCM_DrawString( uint8_t sx, uint8_t row, char *str )
{
	for( int x=0 ; x<strlen(str); x++ )
	{
		LCM_DrawFont( sx+(x*FONT_WIDTH), row, str[x] );
	}
}

void TC3_Overflow(TC_COMPARE_STATUS status, uintptr_t context)
{
    if (status & TC_INTFLAG_OVF_Msk) {
        TC3_IsOverflow = 1;
    }
}

void TC4_Overflow(TC_COMPARE_STATUS status, uintptr_t context)
{
    if (status & TC_INTFLAG_OVF_Msk) {
        TC4_IsOverflow = 1;
    }
}

void USART5_Transmit(uintptr_t context)
{
    USART5_IsTransmitted = 1;
}

void USART5_Receive(uintptr_t context)
{
    USART5_IsReceived = 1;
}

void ADC_Complete(ADC_STATUS status, uintptr_t context) {
    if (status & ADC_INTFLAG_RESRDY_Msk) {
        ADC_Result[ADC_ChannelIdx] = ADC_ConversionResultGet();
        ADC_ChannelIdx = (ADC_ChannelIdx + 1) % 2;
        if (ADC_ChannelIdx == 0) {
            ADC_IsCompleted = 1;
        }
    }
}

int main ( void )
{
    /* Initialize all modules */
    SYS_Initialize ( NULL );

    ADC_CallbackRegister(ADC_Complete, (uintptr_t) NULL);
    ADC_Enable();
    
    TC3_CompareCallbackRegister(TC3_Overflow, (uintptr_t)NULL);
    TC3_CompareStart();

    TC4_CompareCallbackRegister(TC4_Overflow, (uintptr_t)NULL);
    TC4_CompareStart();
    
    SERCOM5_USART_ReadCallbackRegister(USART5_Receive, (uintptr_t) NULL);
    SERCOM5_USART_WriteCallbackRegister(USART5_Transmit, (uintptr_t) NULL);
    SERCOM5_USART_Read(USART5_ReceiveData, 1);
    
    //TODO 14.01
    TCC2_PWMStart();
    
    //TODO 17.03
    SYSTICK_TimerStart();
    SSD1306_CS_PIN_Clear();
    SSD1306_RS_PIN_Clear();
    SERCOM4_SPI_Write((void*) LCM_InitCMD, sizeof(LCM_InitCMD));
    memset(LCM_Blank, 0, 1024);
    
    LCM_DrawBitmap(0, 0, LCM_WIDTH, LCM_HEIGHT, LCM_Blank);
    
    LogoX =127;
    LogoY = 0;
    
    for (i = 0; i < 128; i++) {
        LCM_DrawBitmap(LogoX - i, LogoY, LCM_WIDTH, LCM_HEIGHT, Microchip_Logo);
        SYSTICK_DelayMs((i * i)/400);
    }

    // Delay a while
	SYSTICK_DelayMs(1000);
	// Fill with Blank
	memset(LCM_Blank, 0, 1024);
	LCM_DrawBitmap(0, 0, LCM_WIDTH, LCM_HEIGHT, LCM_Blank);
	// Draw String
	sprintf( (char *)USARTRTxBuffer, "SysClock %ld.%-ldMHz", SYSTICK_TimerFrequencyGet()/1000000, (SYSTICK_TimerFrequencyGet()%1000000)/1000 );
	LCM_DrawString( 0, 0, (char *)USARTRTxBuffer );			
	sprintf( (char *)USARTRTxBuffer, "TC3 period %ldms", TC3_Compare16bitPeriodGet()*1000/TC3_CompareFrequencyGet() );
	LCM_DrawString( 0, 1, (char *)USARTRTxBuffer );			
	sprintf( (char *)USARTRTxBuffer, "TC4 period %ldms", TC4_Compare16bitPeriodGet()*1000/TC4_CompareFrequencyGet() );
	LCM_DrawString( 0, 2, (char *)USARTRTxBuffer );			
	sprintf( (char *)USARTRTxBuffer, "Received Data :" );
	LCM_DrawString( 0, 7, (char *)USARTRTxBuffer );		
    
    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
        
        if (TC3_IsOverflow) {
            
            TC3_IsOverflow = 0;
            LED1_Toggle();
            printf("Hello world!\r\n");
            
            //TODO 17.04
            sprintf((char *)USARTRTxBuffer, "Hello World!!");
            LCM_DrawString(0, 3, (char *)USARTRTxBuffer);
            
            ADC_ConversionStart();
        }
        
        if (TC4_IsOverflow) {
            TC4_IsOverflow = 0;
            LED2_Toggle();
            
            //TODO 16.02
            Duty += DutyDistance;
            if (Duty >= 100 || Duty <= 0) {
                DutyDistance = -DutyDistance;
            }
            
            //TODO 17.05
			sprintf( (char *)USARTRTxBuffer, "PWM Duty : %3d%%", Duty);
			LCM_DrawString( 0, 4, (char *)USARTRTxBuffer );		
            
            if (Duty >= 100) {
                TCC2_PWM16bitDutySet(1, TCC2_PWM16bitPeriodGet() + 1);
            } else {
                TCC2_PWM16bitDutySet(1, ((uint32_t)Duty * TCC2_PWM16bitPeriodGet()) / 100);
            }
        }
        
        if (USART5_IsReceived)
        {
            USART5_IsReceived = 0;
            printf("\r\nReceived Data : %1c\r\n", USART5_ReceiveData[0]);
            SERCOM5_USART_Read(USART5_ReceiveData, 1);
            
            //TODO 17.06
			sprintf( (char *)USARTRTxBuffer, "Received Data : %1c", USART5_ReceiveData[0] );
			LCM_DrawString( 0, 7, (char *)USARTRTxBuffer );
        }
        
        if (ADC_IsCompleted) {
            ADC_IsCompleted = 0;
            
            printf("VR1 Value : %4d\r\n", ADC_Result[0]);
            printf("Temperature Value : %4d\r\n", ADC_Result[1]);
            
            //TODO 17.07
            sprintf((char *)USARTRTxBuffer, "VR1 : %4d", ADC_Result[0]);
            LCM_DrawString(0, 5, (char *)USARTRTxBuffer);
            sprintf((char *)USARTRTxBuffer, "Temp : %4d", ADC_Result[1]);
            LCM_DrawString(0, 6, (char *)USARTRTxBuffer);
            
//            //TODO 15.01
//            if (ADC_Result[0] >= 4095) {
//                TCC2_PWM16bitDutySet(1, TCC2_PWM16bitPeriodGet()+1);
//            } else {
//                TCC2_PWM16bitDutySet(1, (uint32_t)ADC_Result[0] * TCC2_PWM16bitPeriodGet()/4095);
//            }

        }
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}


/*******************************************************************************
 End of File
*/

