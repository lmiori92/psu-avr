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
 * @file display_hal_ncurses.c
 * @author Lorenzo Miori
 * @date May 2016
 * @brief Display HAL to interface the APIs to the UART
 */

#include "display.h"
#include "display_hal.h"

#include "ncurses.h"

#include <stdbool.h>

static void ncurses_display_refresh(void)
{
    refresh();
}

static void ncurses_display_hal_init(void)
{
    initscr(); /* Start curses mode */

    erase();

    ncurses_display_refresh();

}

static void ncurses_display_hal_set_cursor(uint8_t line, uint8_t chr)
{
    move(line + 1, chr+1);

    ncurses_display_refresh();
}

static void ncurses_display_hal_write_char(uint8_t chr)
{
    addch(chr);

    ncurses_display_refresh();
}

static void ncurses_display_hal_cursor_visibility(bool visible)
{
    curs_set((visible == TRUE) ? 1U : 0U);

    ncurses_display_refresh();
}

/**
 *
 * @brief
 *
 * Register the display to the display subsystem HAL
 *
 * @param   funcs   the function pointer structure
 */
void ncurses_set_hal(t_display_hal_functions *funcs)
{

    funcs->display_hal_init = ncurses_display_hal_init;
    funcs->display_hal_set_cursor = ncurses_display_hal_set_cursor;
    funcs->display_hal_write_char = ncurses_display_hal_write_char;
    funcs->display_hal_cursor_visibility = ncurses_display_hal_cursor_visibility;

}
