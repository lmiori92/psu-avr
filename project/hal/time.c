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
 * @file time.c
 * @author Lorenzo Miori
 * @date Oct 2015
 * @brief Time keeping routines
 */

#include "stdbool.h"

/* avr libs */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "inc/time_m.h"

#define TIMER_0_PRESCALER_8     (1 << CS01)                     /**< Prescaler 8 */
#define TIMER_0_PRESCALER_64    ((1 << CS01) | (1 << CS00))     /**< Prescaler 64 */
#define TIMER_0_PRESCALER_256   (1 << CS02)                     /**< Prescaler 256 */

/*
 * Clock is 16MHz, with a prescaler of 8: 2MHz timer clock.
 * So, a complete overflow of the 8-bit register happens every: 128us
 * Setting the compare value to 200 allows us to interrupt every exactly 100us
 *
 * */

volatile uint32_t g_timestamp;           /**< Global time-keeping variable (resolution: 100us) */

/**
 * timer_init
 *
 * @brief Initialize timer, interrupt and variable.
 *        Assumes that interrupts are disabled while intializing
 *
 */
void timer_init(void)
{

    /* CTC mode */
    TCCR0A = (1 << WGM01);
    /* set up timer with prescaler */
    TCCR0B = TIMER_0_PRESCALER_8;

    /* initialize counter */
    OCR0A = 200;

    /* enable output compare match interrupt */
    TIMSK0 |= (1 << OCIE0A);

}

#ifdef TIMER_DEBUG
#include "stdio.h"
void timer_debug(void)
{
    static uint32_t ts = 0;
    if (g_timestamp > (ts + 1000000L))
    {
        ts = g_timestamp;
        printf("%s\r\n", "1 second trigger");
    }

}
#else
/* No function is available */
#endif

void timer_delay_ms(uint16_t ms)
{
    while(ms--)
    {
        _delay_ms(1);
    }
}

/**
 * ISR(TIMER0_OVF_vect)
 *
 * @brief Timer comparator interrupt routine
 * */
ISR(TIMER0_COMPA_vect)
{
    g_timestamp += 100U;   	/* 100us */
}

