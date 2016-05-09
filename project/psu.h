/*
 * psu.h
 *
 *  Created on: 08 mag 2016
 *      Author: lorenzo
 */

#ifndef PSU_H_
#define PSU_H_

#include <stdint.h>

typedef uint16_t t_value_type;

typedef struct
{
    t_value_type raw;
    t_value_type scaled;
} t_value;

typedef struct _t_value_scale
{
    t_value_type min;
    t_value_type max;
    t_value_type min_scaled;
    t_value_type max_scaled;
} t_value_scale;

typedef struct _t_voltage
{
    t_value_scale scale;
    t_value       value;        /**< Voltage [mV] */
} t_voltage;

typedef struct _t_current
{
    t_value_scale scale;
    t_value value;     /**< Current [mA] */
} t_current;

typedef struct _t_channel
{
    uint8_t remote_node;        /**< 0: local node; >1: remote node monitoring */

    t_voltage voltage_setpoint;
    t_voltage voltage_readout;
    e_adc_channel voltage_adc_channel;
    e_pwm_channel voltage_pwm_channel;

    t_current current_setpoint;
    t_current current_readout;
    e_adc_channel current_adc_channel;
    e_pwm_channel current_pwm_channel;
} t_psu_channel;

typedef enum _e_psu_channels
{
    PSU_CHANNEL_0,
    PSU_CHANNEL_1,

    PSU_CHANNEL_NUM
} e_psu_channel;

#endif /* PSU_H_ */
