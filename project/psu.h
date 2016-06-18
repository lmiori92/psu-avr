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

#include "adc.h"
#include "pwm.h"
#include "lib.h"

#include <stdint.h>

#define PSU_CHANNEL_TIMEOUT   2000000U    /**< Timeout if commnuication link broken or erroneus */

typedef enum
{
    PSU_SETPOINT_VOLTAGE,
    PSU_SETPOINT_CURRENT,
} e_psu_setpoint;

typedef enum
{
    PSU_STATE_INIT,
    PSU_STATE_PREOPERATIONAL,
    PSU_STATE_OPERATIONAL,
    PSU_STATE_SAFE_STATE
} e_psu_state;

typedef struct _t_voltage
{
    t_value_scale scale;
    t_value       value;        /**< Voltage [mV] / Current [mA] / ... */
} t_measurement;

typedef struct _t_channel
{
    uint8_t        node_id;              /**< 0: local node; >1: remote node monitoring */
    bool           remote_or_local;      /**< 0: local; 1: remote */
    bool           master_or_slave;      /**< */
    e_psu_state    state;                   /**< PSU Channel state */
    uint32_t       heartbeat_timestamp;     /**< Store the timestamp when the last communication with the slave happened */

    t_measurement     voltage_setpoint;     /**< Voltage setpoint for the channel */
    t_measurement     voltage_readout;      /**< Voltage measurement for the channel */
    e_adc_channel  voltage_adc_channel;     /**< Voltage measurement ADC channel */
    e_pwm_channel  voltage_pwm_channel;     /**< Voltage output PWM channel */

    t_measurement      current_setpoint;    /**< Current setpoint for the channel */
    t_measurement      current_readout;     /**< Current measurement for the channel */
    e_adc_channel  current_adc_channel;     /**< Current measurement ADC channel */
    e_pwm_channel  current_pwm_channel;     /**< Current output PWM channel */
} t_psu_channel;

typedef enum _e_psu_channels
{
    PSU_CHANNEL_0,
    PSU_CHANNEL_1,

    PSU_CHANNEL_NUM
} e_psu_channel;

void psu_app_init(void);
void psu_app(void);

#endif /* PSU_H_ */
