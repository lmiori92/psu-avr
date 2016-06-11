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
 * @file display_config.h
 * @author Lorenzo Miori
 * @date June 2016
 * @brief Display subsystem configuration (mainly subsystem selection)
 */

#ifndef DISPLAY_CONFIG_H_
#define DISPLAY_CONFIG_H_

#include "display.h"
#include "display_hal.h"

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

void display_select(void);

#endif /* DISPLAY_CONFIG_H_ */
