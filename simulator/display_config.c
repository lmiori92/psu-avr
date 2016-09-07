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
 * @file display_config.c
 * @author Lorenzo Miori
 * @date June 2016
 * @brief Display subsystem configuration (mainly subsystem selection)
 */

#include "display.h"
#include "display_hal.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>  /* printf-like facility */

void display_select(void)
{
    /* That is what we will use on the final project */
    ncurses_set_hal(display_get_current_hal());
}
