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
 * @file remote.c
 * @author Lorenzo Miori
 * @date May 2016
 * @brief Encoder subsystem: initialization and routines to handle them
 */

#ifndef PSU_H_
#define PSU_H_

#include "psu.h"
#include "inc/adc.h"
#include "inc/pwm.h"
#include "lorenzlib/lib.h"
#include "pid.h"

#include <stdint.h>

typedef enum _e_psu_setpoint
{
    PSU_SETPOINT_VOLTAGE,
    PSU_SETPOINT_CURRENT,
} e_psu_setpoint;

typedef enum _e_psu_gui_menu
{
    PSU_MENU_PSU,
    PSU_MENU_MAIN,
    PSU_MENU_STARTUP
} e_psu_gui_menu;

typedef struct
{
    bool     flag;
    uint32_t timestamp;
} t_timer32;

typedef struct
{
    e_psu_channel  selected_psu;
    e_psu_setpoint selected_setpoint;

    t_psu_channel  *selected_psu_ptr;           /**< Keep a reference to the selected PSU for optimization in the ISR callback */
    t_psu_setpoint  *selected_setpoint_ptr;      /**< Keep a reference to the selected PSU for optimization in the ISR callback */

    bool           master_or_slave;             /**< True: is a master; False: is a slave */

    t_timer32        flag_50ms;                   /**< True when 50ms have been passed; False: the immediate next cycle */
    uint32_t       cycle_time;                  /**< Cycle Time (main application thread) */
    uint32_t       cycle_time_max;
} t_application;

void psu_app_init(void);
void psu_app(void);

#endif /* PSU_H_ */
