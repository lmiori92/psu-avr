/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Lorenzo Miori (C) 2015 [ 3M4|L: memoryS60<at>gmail.com ]

    Version History
        * 1.0 initial

*/

/**
 * @file system.c
 * @author Lorenzo Miori
 * @date Oct 2015
 * @brief System level utilities: ISR debugging / MCU power state
 */

#include <avr/interrupt.h>
#include <avr/io.h>

#include "uart.h"
#include "system.h"

/* http://www.avrfreaks.net/forum/soft-c-avrgcc-monitoring-stack-usage */
#ifdef STACK_MONITORING
void StackPaint(void)
{
#if 0
    uint8_t *p = &_end;

    while(p <= &__stack)
    {
        *p = STACK_CANARY;
        p++;
    }
#else
    __asm volatile ("    ldi r30,lo8(_end)\n"
                    "    ldi r31,hi8(_end)\n"
                        "    ldi r24,lo8(0xc5)\n" /* STACK_CANARY = 0xc5 */
                        "    ldi r25,hi8(__stack)\n"
                        "    rjmp .cmp\n"
                        ".loop:\n"
                        "    st Z+,r24\n"
                        ".cmp:\n"
                        "    cpi r30,lo8(__stack)\n"
                        "    cpc r31,r25\n"
                        "    brlo .loop\n"
                        "    breq .loop"::);
    #endif
    }

uint16_t StackCount(void)
{
    const uint8_t *p = &_end;
    uint16_t       c = 0;

    while(*p == 0xc5 && p <= &__stack)
    {
        p++;
        c++;
    }

    return c;
}
#endif

/**
 * ISR(BADISR_vect)
 *
 * @brief This interrupt handler is executed whenever an ISR is fired
 * without a defined ISR routine.
 * It tries to write a string on the display and then blocks.
 * Especially useful when implementing interrupt routines..
 */
ISR(BADISR_vect)
{
    uart_putstring("no ISR!\r\n");
    for(;;);
}

void system_fatal(char *str)
{
    uart_putstring(str);
    for(;;);
}

void system_reset(void)
{
    /* start at zero! */
    void (*start)(void) = 0;
    start();
}

/**
 *
 * system_init
 *
 * @brief System init
 *
 */
uint8_t system_init(void)
{
//    if(MCUCSR & (1<<PORF )) (PSTR("Power-on reset.\n"));
//    if(MCUCSR & (1<<EXTRF)) (PSTR("External reset!\n"));
//    if(MCUCSR & (1<<BORF )) (PSTR("Brownout reset!\n"));
//    if(MCUCSR & (1<<WDRF )) (PSTR("Watchdog reset!\n"));
    uint8_t t = MCUSR;

    /* Reset state for the next proper detection */
    MCUSR = 0;

    /* CODING PIN (MASTER/SLAVE) configuration */
    CODING_CONFIG;

    return t;
}

/**
 *
 * system_coding_pin_read
 *
 * @brief Read the 1-bit coding model pin(s)
 *
 * @return  TRUE or FALSE (1-bit) value to be used by the logic
 *
 */
bool system_coding_pin_read(void)
{
    return !!CODING_READ;
}
