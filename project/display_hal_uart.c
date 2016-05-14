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

    Lorenzo Miori (C) 2016 [ 3M4|L: memoryS60<at>gmail.com ]

    Version History
        * 1.0 initial

*/

/**
 * @file display_hal_uart.c
 * @author Lorenzo Miori
 * @date May 2016
 * @brief Display HAL to interface the APIs to the UART
 */

#include "display.h"
#include "uart.h"

#include <stdbool.h>
#include <stdio.h>  // could be also removed and simplified

void display_hal_init_DISABLED(void)
{
    uart_putstring("\x1B[2J\x1B[H");
}

void display_hal_set_cursor_DISABLED(uint8_t line, uint8_t chr)
{
    /* Line 0 is 1 in UART; Line 1 is 2 in UART and so on ... */
    printf("\x1B[%d;%dH", line + 1U, chr + 1U);
}

void display_hal_write_char_DISABLED(uint8_t chr)
{
    uart_putchar(chr, NULL);
}

void display_hal_cursor_visibility_DISABLED(bool visible)
{
    if (visible == false)
        uart_putstring("\x1B[?25l");
    else
        uart_putstring("\x1B[?25h");
}
