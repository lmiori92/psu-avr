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
 * @file psu.h
 * @author Lorenzo Miori
 * @date May 2016
 * @brief Power Supply subsystem: manage power supply units (PSUs)
 */

#ifndef APP_PSU_H_
#define APP_PSU_H_

#include "pid.h"
#include "lorenzlib/lib.h"

/* System includes */
#include <stdbool.h>

#define PSU_TIMEOUT_TICKS   10U    /**< Timeout when error/HW lockup (in ticks) */

typedef enum _e_psu_adc_channel
{
    PSU_ADC_0,
    PSU_ADC_1,
    PSU_ADC_2,
    PSU_ADC_3,
    PSU_ADC_4,
    PSU_ADC_5,
    PSU_ADC_6,
    PSU_ADC_7,
    PSU_ADC_8,
    PSU_ADC_9,
    PSU_ADC_10,
    PSU_ADC_11,
    PSU_ADC_12,
    PSU_ADC_13,
    PSU_ADC_14,
    PSU_ADC_15,

    PSU_ADC_NUM,

    PSU_ADC_NONE
} e_psu_ADC_channel;

typedef enum _e_psu_dac_channel
{
    PSU_DAC_0,
    PSU_DAC_1,
    PSU_DAC_2,
    PSU_DAC_3,
    PSU_DAC_4,
    PSU_DAC_5,
    PSU_DAC_6,
    PSU_DAC_7,
    PSU_DAC_8,
    PSU_DAC_9,
    PSU_DAC_10,
    PSU_DAC_11,
    PSU_DAC_12,
    PSU_DAC_13,
    PSU_DAC_14,
    PSU_DAC_15,

    PSU_DAC_NUM,

    PSU_DAC_NONE
} e_psu_DAC_channel;

typedef enum _e_psu_channels
{
    PSU_CHANNEL_0,
    PSU_CHANNEL_1,

    PSU_CHANNEL_NUM
} e_psu_channel;

typedef enum _e_psu_state
{
    PSU_STATE_INIT,
    PSU_STATE_PREOPERATIONAL,
    PSU_STATE_OPERATIONAL,
    PSU_STATE_SAFE_STATE
} e_psu_state;

typedef struct _t_psu_readout
{
    t_value_scale       scale;
    t_value             value;        /**< Voltage [mV] / Current [mA] / ... */
    t_low_pass_filter   filter;
} t_measurement;

typedef struct _t_psu_setpoint
{
    t_value_scale scale;
    t_value       value;        /**< Voltage [mV] / Current [mA] / ... */
} t_psu_setpoint;

typedef struct _t_channel
{
    uint8_t        node_id;              /**< 0: local node; >1: remote node monitoring */
    bool           remote_or_local;      /**< 0: local; 1: remote */
    e_psu_state    state;                   /**< PSU Channel state */
    uint8_t       heartbeat_ticks;     /**< Timer for the PSU channel communication (or error)s */

    t_psu_setpoint     voltage_setpoint;     /**< Voltage setpoint for the channel */
    t_measurement     voltage_readout;      /**< Voltage measurement for the channel */

    t_psu_setpoint      current_setpoint;    /**< Current setpoint for the channel */
    t_measurement      current_readout;     /**< Current measurement for the channel */


//    e_psu_ADC_channel  voltage_adc_channel;     /**< Voltage measurement ADC channel */
//    e_psu_DAC_channel  voltage_dac_channel;     /**< Voltage output PWM channel */
//    e_psu_ADC_channel  current_adc_channel;     /**< Current measurement ADC channel */
//    e_psu_DAC_channel  current_dac_channel;     /**< Current output PWM channel */
    pidData_t       current_limit_pid;      /**< Current limit mode PID controller state */

} t_psu_channel;

typedef enum
{
    DATATYPE_NULL,       /**< Datagram Header Only (data len = 0) */
    DATATYPE_CONFIG,     /**< Datagram Data carries config data */
    DATATYPE_READOUTS,   /**< Datagram Data carries readouts data */
    DATATYPE_SETPOINTS,  /**< Datagram Data carries setpoints data */
    DATAYPE_DEBUG        /**< Datagram Data carries debug data */
} e_datatype;

void psu_init_channel(t_psu_channel *channel, e_psu_channel psu_ch, bool master_or_slave);
void remote_decode_datagram(t_psu_channel *channels);
void remote_encode_datagram(e_datatype type, t_psu_channel *channel);
void psu_check_channel(t_psu_channel *psu_channel, bool tick);

void psu_set_measurement(t_measurement *measurement, t_value_type value);
void psu_set_setpoint(t_psu_setpoint *setpoint, t_value_type value);

void psu_set_setpoint_scale(t_psu_setpoint *setpoint,
        t_value_type min, t_value_type max,
        t_value_type min_scaled, t_value_type max_scaled);

void psu_set_measurement_scale(t_measurement *setpoint,
        t_value_type min, t_value_type max,
        t_value_type min_scaled, t_value_type max_scaled);

#endif /* APP_PSU_H_ */
