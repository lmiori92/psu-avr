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
 * @file time.h
 * @author Lorenzo Miori
 * @date Oct 2015
 * @brief Time keeping routines
 */

#ifndef SRC_TIME_H_
#define SRC_TIME_H_

/*#define TIMER_DEBUG*/

extern volatile uint32_t g_timestamp;    /**< Time-keeping in us. Resolution is 100us TICK */

void timer_init(void);
void timer_delay_ms(uint16_t ms);

#ifdef TIMER_DEBUG
void timer_debug(void);
#endif

#endif /* SRC_TIME_H_ */
