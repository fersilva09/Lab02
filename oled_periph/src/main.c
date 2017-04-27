/*****************************************************************************
 *   Peripherals such as temp sensor, light sensor, accelerometer,
 *   and trim potentiometer are monitored and values are written to
 *   the OLED display.
 *
 *   Copyright(C) 2009, Embedded Artists AB
 *   All rights reserved.
 *
 ******************************************************************************/

/*
#include <stdio.h>
//#include "mcu_regs.h"
#include "lpc_types.h"
#include "lpc13xx_uart.h"
#include "lpc13xx_timer.h"
#include "lpc13xx_i2c.h"
#include "lpc13xx_gpio.h"
#include "lpc13xx_ssp.h"
#include "lpc13xx_adc.h"
*/
#include "mcu_regs.h"
#include "type.h"
#include "uart.h"
#include "stdio.h"
#include "timer32.h"
#include "i2c.h"
#include "gpio.h"
#include "ssp.h"
#include "adc.h"

#include "light.h"
#include "oled.h"
#include "temp.h"
#include "acc.h"


static uint32_t msTicks = 0;
static uint8_t buf[10];

static void intToString(int value, uint8_t* pBuf, uint32_t len, uint32_t base)
{
    static const char* pAscii = "0123456789abcdefghijklmnopqrstuvwxyz";
    int pos = 0;
    int tmpValue = value;

    // the buffer must not be null and at least have a length of 2 to handle one
    // digit and null-terminator
    if (pBuf == NULL || len < 2)
    {
        return;
    }

    // a valid base cannot be less than 2 or larger than 36
    // a base value of 2 means binary representation. A value of 1 would mean only zeros
    // a base larger than 36 can only be used if a larger alphabet were used.
    if (base < 2 || base > 36)
    {
        return;
    }

    // negative value
    if (value < 0)
    {
        tmpValue = -tmpValue;
        value    = -value;
        pBuf[pos++] = '-';
    }

    // calculate the required length of the buffer
    do {
        pos++;
        tmpValue /= base;
    } while(tmpValue > 0);


    if (pos > len)
    {
        // the len parameter is invalid.
        return;
    }

    pBuf[pos] = '\0';

    do {
        pBuf[--pos] = pAscii[value % base];
        value /= base;
    } while(value > 0);

    return;

}

void SysTick_Handler(void) {
    msTicks++;
}

static uint32_t getTicks(void)
{
    return msTicks;
}


int main (void)
{
    //SystemInit();
    SystemCoreClockUpdate();

    int32_t xoff = 0;
    int32_t yoff = 0;
    int32_t zoff = 0;

    int8_t x = 0;
    int8_t y = 0;
    int8_t z = 0;

    int32_t t = 0;
    uint32_t lux = 0;
    uint32_t trim = 0;

    GPIOInit();
    init_timer32(0, 10);

    UARTInit(115200);
    UARTSendString((uint8_t*)"OLED - Peripherals\r\n");

    I2CInit( (uint32_t)I2CMASTER, 0 );
    SSPInit();
    ADCInit( ADC_CLK );

    oled_init();
    light_init();
    acc_init();

    temp_init (&getTicks);


    /* setup sys Tick. Elapsed time is e.g. needed by temperature sensor */
    SysTick_Config(SystemCoreClock / 1000);
    if ( !(SysTick->CTRL & (1<<SysTick_CTRL_CLKSOURCE_Msk)) )
    {
      /* When external reference clock is used(CLKSOURCE in
      Systick Control and register bit 2 is set to 0), the
      SYSTICKCLKDIV must be a non-zero value and 2.5 times
      faster than the reference clock.
      When core clock, or system AHB clock, is used(CLKSOURCE
      in Systick Control and register bit 2 is set to 1), the
      SYSTICKCLKDIV has no effect to the SYSTICK frequency. See
      more on Systick clock and status register in Cortex-M3
      technical Reference Manual. */
      LPC_SYSCON->SYSTICKCLKDIV = 0x08;
    }

    /*
     * Assume base board in zero-g position when reading first value.
     */
    acc_read(&x, &y, &z);
    xoff = 0-x;
    yoff = 0-y;
    zoff = 64-z;

    light_enable();
    light_setRange(LIGHT_RANGE_4000);
    
     //eixo x e eixo y
    oled_clearScreen(OLED_COLOR_BLACK);
    oled_line(0, 32, 96, 32, OLED_COLOR_WHITE);
    oled_line(0, 0, 0, 64, OLED_COLOR_WHITE);
    
    //escalas no eixo x em 10 divisões
    oled_line(10, 31, 10, 33, OLED_COLOR_WHITE);
    oled_line(20, 31, 20, 33, OLED_COLOR_WHITE);
    oled_line(30, 31, 30, 33, OLED_COLOR_WHITE);
    oled_line(40, 31, 40, 33, OLED_COLOR_WHITE);
    oled_line(50, 31, 50, 33, OLED_COLOR_WHITE);
    oled_line(60, 31, 60, 33, OLED_COLOR_WHITE);
    oled_line(70, 31, 70, 33, OLED_COLOR_WHITE);
    oled_line(80, 31, 80, 33, OLED_COLOR_WHITE);
    oled_line(90, 31, 90, 33, OLED_COLOR_WHITE);
    oled_line(96, 31, 96, 33, OLED_COLOR_WHITE);
    
     //escalas no eixo y em 6 divisões
    oled_line(0, 38, 1, 38, OLED_COLOR_WHITE);
    oled_line(0, 44, 1, 44, OLED_COLOR_WHITE);
    oled_line(0, 50, 1, 50, OLED_COLOR_WHITE);
    oled_line(0, 56, 1, 56, OLED_COLOR_WHITE);
    oled_line(0, 62, 1, 62, OLED_COLOR_WHITE);
    oled_line(0, 26, 1, 26, OLED_COLOR_WHITE);
    oled_line(0, 20, 1, 20, OLED_COLOR_WHITE);
    oled_line(0, 14, 1, 14, OLED_COLOR_WHITE);
    oled_line(0, 8, 1, 8, OLED_COLOR_WHITE);
    oled_line(0, 2, 1, 2, OLED_COLOR_WHITE);
    


    /*oled_putString(1,1,  (uint8_t*)"Temp   : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1,9,  (uint8_t*)"Light  : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1,17, (uint8_t*)"Trimpot: ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1,25, (uint8_t*)"Acc x  : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1,33, (uint8_t*)"Acc y  : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1,41, (uint8_t*)"Acc z  : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
*/
    while(1) {

       
        /* Accelerometer 
        acc_read(&x, &y, &z);
        x = x+xoff;
        y = y+yoff;
        z = z+zoff;
*/
        /* Temperature */
       // t = temp_read();

        /* light */
       // lux = light_read();

        /* trimpot */
      // trim = ADCRead(0);

        /* output values to OLED display 

        intToString(t, buf, 10, 10);
        oled_fillRect((1+9*6),1, 80, 8, OLED_COLOR_WHITE);
        oled_putString((1+9*6),1, buf, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

        intToString(lux, buf, 10, 10);
        oled_fillRect((1+9*6),9, 80, 16, OLED_COLOR_WHITE);
        oled_putString((1+9*6),9, buf, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

        intToString(trim, buf, 10, 10);
        oled_fillRect((1+9*6),17, 80, 24, OLED_COLOR_WHITE);
        oled_putString((1+9*6),17, buf, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

        intToString(x, buf, 10, 10);
        oled_fillRect((1+9*6),25, 80, 32, OLED_COLOR_WHITE);
        oled_putString((1+9*6),25, buf, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

        intToString(y, buf, 10, 10);
        oled_fillRect((1+9*6),33, 80, 40, OLED_COLOR_WHITE);
        oled_putString((1+9*6),33, buf, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

        intToString(z, buf, 10, 10);
        oled_fillRect((1+9*6),41, 80, 48, OLED_COLOR_WHITE);
        oled_putString((1+9*6),41, buf, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
        */
        /* delay */
        delay32Ms(0, 200);
  
    }

}
