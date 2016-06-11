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

void system_fatal(char *str)
{
    for(;;);
}

void system_reset(void)
{
    /* do nothing */
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
    return 0;
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
extern bool g_master_or_slave;
bool system_coding_pin_read(void)
{
    return g_master_or_slave;
}
