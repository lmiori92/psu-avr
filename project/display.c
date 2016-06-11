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
 * @file display.h
 * @author Lorenzo Miori
 * @date May 2016
 * @brief Display subsystem: buffering and display primitives
 */

#include "display.h"
#include "display_hal.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>  /* printf-like facility */

/*#define DISPLAY_HAS_PRINTF*/

static t_display_status display_status;
static t_display_elem   display_buffer[DISPLAY_LINE_NUM][DISPLAY_CHAR_NUM];

#ifdef DISPLAY_HAS_PRINTF
static char snprintf_buf[DISPLAY_CHAR_NUM*DISPLAY_LINE_NUM];
#endif

static t_display_hal_functions display_hal_functions;   /**< HAL implementation in use (no NULL pointer check is performed!!) */

t_display_hal_functions* display_get_current_hal(void)
{
    return &display_hal_functions;
}

void display_init(void)
{
    (*display_hal_functions.display_hal_init)();
    display_set_cursor(0, 0);
}

void display_clear(uint8_t lines)
{
    uint8_t i,j;
    for (i = 0; i < DISPLAY_LINE_NUM; i++)
    {
        if (((lines >> i) & 0x1U) == 0x1U)
        {
            for (j = 0; j < DISPLAY_CHAR_NUM; j++)
            {
                display_buffer[i][j].character = (uint8_t)' ';          /* space in the current buffer */
                display_buffer[i][j].character_prev = (uint8_t)'\0';    /* zero the previous buffer to force a complete redraw */
            }
        }
    }
}

void display_clear_all(void)
{
    display_clear(0xFFU);
}

void display_periodic(void)
{
    uint8_t i, j;

    for (i = 0; i < DISPLAY_LINE_NUM; i++)
    {
        for (j = 0; j < DISPLAY_CHAR_NUM; j++)
        {
            if (display_buffer[i][j].character != display_buffer[i][j].character_prev)
            {
                display_buffer[i][j].character_prev = display_buffer[i][j].character;
                (*display_hal_functions.display_hal_set_cursor)(i, j);
                (*display_hal_functions.display_hal_write_char)(display_buffer[i][j].character);
           }
        }
    }

}

void display_set_cursor(uint8_t line, uint8_t chr)
{
    display_status.line_n = (line < DISPLAY_LINE_NUM) ? line : (DISPLAY_LINE_NUM - 1U);
    display_status.char_n = (chr  < DISPLAY_CHAR_NUM) ? chr  : (DISPLAY_CHAR_NUM - 1U);
}

void display_advance_cursor(uint8_t num)
{
    uint8_t tmp;
    uint8_t dline;
    uint8_t dchar;

    tmp = (display_status.line_n * DISPLAY_CHAR_NUM) + display_status.char_n + num;
    if (tmp >= (DISPLAY_CHAR_NUM * DISPLAY_LINE_NUM))
    {
        dchar = DISPLAY_CHAR_NUM - 1U;
        dline = DISPLAY_LINE_NUM - 1U;
    }
    else
    {
        dchar = tmp % DISPLAY_CHAR_NUM;
        dline = tmp / DISPLAY_CHAR_NUM;
    }

    display_set_cursor(dline, dchar);

}

void display_enable_cursor(bool visible)
{
    (*display_hal_functions.display_hal_cursor_visibility)(visible);
}

void display_write_char(uint8_t chr)
{
    /* add char to buffer */
    display_buffer[display_status.line_n][display_status.char_n].character = chr;
    /* advance the cursor */
    display_advance_cursor(1U);
}

void display_write_string(char *str)
{
    while (*str != '\0')
    {
        display_write_char(*str);
        str++;
    }
}

void display_write_stringf(char *fmt, ...)
{
#ifdef DISPLAY_HAS_PRINTF
    va_list va;
    va_start(va,fmt);
    vsnprintf(snprintf_buf, sizeof(snprintf_buf), fmt, va);
    va_end(va);
    display_write_string(snprintf_buf);
#else
    (void)fmt;
#endif
}
