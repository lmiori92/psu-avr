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
 * @file error.h
 * @author Lorenzo Miori
 * @date May 2016
 * @brief Error codes and handling
 */

#ifndef ERROR_H_
#define ERROR_H_

typedef enum _e_error
{
    E_OK,
    E_UNKNOWN,
    E_CRC,
    E_OVERFLOW,
    E_TIMEOUT,
} e_error;

#endif /* ERROR_H_ */
