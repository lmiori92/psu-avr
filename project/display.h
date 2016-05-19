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

#ifndef DISPLAY_H_
#define DISPLAY_H_

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

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#define DISPLAY_LINE_NUM         2U
#define DISPLAY_CHAR_NUM        16U

typedef struct
{
    uint8_t line_n;
    uint8_t char_n;
} t_display_status;

typedef struct
{
    uint8_t character;
    uint8_t character_prev;
} t_display_elem;

/* Display APIs */
void display_init(void);
void display_clear(uint8_t lines);
void display_clear_all(void);
void display_periodic(void);
void display_set_cursor(uint8_t line, uint8_t chr);
void display_enable_cursor(bool visible);
void display_advance_cursor(uint8_t num);
void display_write_char(uint8_t chr);
void display_write_string(char *str);
void display_write_stringf(char *fmt, ...);
void display_write_number(uint8_t number);

#endif /* DISPLAY_H_ */
