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
 * @file display_hal.h
 * @author Lorenzo Miori
 * @date May 2016
 * @brief Display HAL primitives
 */

#ifndef DISPLAY_HAL_H_
#define DISPLAY_HAL_H_

#include <stdbool.h>
#include <stdint.h>

/** Hardware Abstraction Layer */
typedef struct
{
    void (*display_hal_init) (void);
    void (*display_hal_set_cursor) (uint8_t, uint8_t);
    void (*display_hal_write_char) (uint8_t);
    void (*display_hal_cursor_visibility) (bool);
} t_display_hal_functions;

t_display_hal_functions* display_get_current_hal(void);

/* HAL implementations for various kind of display types */
void uart_set_hal(t_display_hal_functions *funcs);
void hd44780_set_hal(t_display_hal_functions *funcs);
void ncurses_set_hal(t_display_hal_functions *funcs);

#endif /* DISPLAY_HAL_H_ */
