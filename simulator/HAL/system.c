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
