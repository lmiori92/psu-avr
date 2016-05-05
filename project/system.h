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
 * @file system.h
 * @author Lorenzo Miori
 * @date Oct 2015
 * @brief System level utilities: ISR debugging / MCU power state
 */

#ifndef SRC_SYSTEM_H_
#define SRC_SYSTEM_H_

/*#define STACK_MONITORING*/    /**< Enable stack monitoring */

#include <stdint.h>

/** Output values */
typedef struct
{

    uint8_t relays;    /**< bits representing relay state */

} t_output;

/** Operational variables */
typedef struct
{

    uint32_t cycle_time;        /**< Time it takes the logic to execute */
    uint32_t cycle_time_max;    /**< Maximum time it took the logic to execute */
    t_output output;            /**< State of the outputs */
    uint8_t  reset_reason;      /**< Reset reason (see datasheet) */

} t_operational;

/* Globals */
extern uint8_t _end;
extern uint8_t __stack;

/* Functions */
uint8_t system_init(void);
void system_fatal(char *str);
void system_reset(void);
void StackPaint(void) __attribute__ ((naked)) __attribute__ ((section (".init1")));

#define DBG_LOW      PORTD &= ~(1 << PIN2);asm("nop")  /**< DBG LOW */
#define DBG_HIGH     PORTD |=  (1 << PIN2);asm("nop")  /**< DBG HIGH */
#define DBG_CONFIG   DDRD |= (1 << PIN2)

#endif /* SRC_SYSTEM_H_ */
