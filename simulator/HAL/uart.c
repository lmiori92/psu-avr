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

#include "uart.h"

#include <stdio.h>
#include <string.h>

FILE uart_output;
FILE uart_input;

static t_uart_cb uart_cb = NULL;

void uart_init(void)
{
    memcpy(&uart_output, stdout, sizeof(FILE));
    memcpy(&uart_input, stdin, sizeof(FILE));
}

void uart_callback(t_uart_cb cb)
{
    uart_cb = cb;
}

void uart_putchar(char c, FILE *stream)
{

}

void uart_putstring(char *str)
{
    while (*str != 0)
    {
        uart_putchar(*str, NULL);
        str++;
    }
}

char uart_getchar(FILE *stream)
{
    return (char)0;
}
