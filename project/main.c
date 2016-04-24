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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/delay.h>
#include "adc.h"
#include "pwm.h"
#include "time.h"
#include "uart.h"
#include "system.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"

/* HIGH */

// All the control algorithms and (graphical) user interface

/* MIDDLE */

//#define t_value_type uint16_t;
typedef uint16_t t_value_type;

typedef struct
{
    t_value_type raw;
    t_value_type scaled;
} t_value;

typedef struct _t_value_scale
{
    t_value_type scale;
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
    t_voltage voltage_setpoint;
    t_voltage voltage_readout;
    e_adc_channel voltage_adc_channel;
    e_pwm_channel voltage_pwm_channel;

    t_current current_setpoint;
    t_current current_readout;
    e_adc_channel current_adc_channel;
    e_pwm_channel current_pwm_channel;
} t_psu_channel;


/* LOW */

typedef struct
{
    uint8_t  resolution;
    uint16_t duty_cycle;
} t_pwm_out;

typedef enum _e_psu_channels
{
    PSU_CHANNEL_0,
    PSU_CHANNEL_1,

    PSU_CHANNEL_NUM
} e_psu_channel;

/* GLOBALS */
static t_psu_channel psu_channels[PSU_CHANNEL_NUM];

void lib_limit(t_value *value, t_value_scale *scale)
{
    /* bottom filter */
    if (value->raw < scale->min) value->scaled = scale->min;
    /* top filter */
    if (value->raw > scale->max) value->scaled = scale->max;
}

void lib_scale(t_value *value, t_value_scale *scale)
{
    t_value_type temp;
    float range_ratio = ((float)scale->max_scaled - scale->min_scaled) / ((float)scale->max - scale->min);

    /* Remove the min value from the raw value */
    temp = value->raw - scale->min;
    /* Multiply the raw value by the ratio between the destination range and the origin range */
    temp = (float)temp * range_ratio;
    /* Offset the value by the min scaled value */
    temp += scale->min_scaled;
    /* Assign the value to the output */
    value->scaled = (t_value_type)temp;
}

static void init_channel(t_psu_channel *channel, e_psu_channel psu_ch)
{

    switch(psu_ch)
    {
        case PSU_CHANNEL_0:
            channel->voltage_adc_channel = ADC_0;
            channel->current_adc_channel = ADC_1;
            channel->voltage_pwm_channel = PWM_CHANNEL_0;
            channel->current_pwm_channel = PWM_CHANNEL_2;
            break;
        case PSU_CHANNEL_1:
            channel->voltage_adc_channel = ADC_2;
            channel->current_adc_channel = ADC_3;
            channel->voltage_pwm_channel = PWM_CHANNEL_1;
            channel->current_pwm_channel = PWM_CHANNEL_3;
            break;
        default:
            /* No channel selected */
            break;
    }

    channel->voltage_readout.scale.min = 0;
    channel->voltage_readout.scale.max = 1023;  /* ADC steps */
    channel->voltage_readout.scale.min_scaled = 0;
    channel->voltage_readout.scale.max_scaled = 5000;//25575;  /* Voltage */

    channel->current_readout.scale.min = 0;
    channel->current_readout.scale.max = 1023;  /* ADC steps */
    channel->current_readout.scale.min_scaled = 0;
    channel->current_readout.scale.max_scaled = 2048;  /* Voltage */

    channel->voltage_setpoint.scale.min = channel->voltage_readout.scale.min_scaled;
    channel->voltage_setpoint.scale.max = 28500;
    channel->voltage_setpoint.scale.min_scaled = 0;
    channel->voltage_setpoint.scale.max_scaled = pwm_get_resolution(channel->voltage_pwm_channel);

    channel->current_setpoint.scale.min = 0;
    channel->current_setpoint.scale.max = 28500;
    channel->current_setpoint.scale.min_scaled = 0;
    channel->current_setpoint.scale.max_scaled = pwm_get_resolution(channel->current_pwm_channel);

}

static void init_psu(t_psu_channel *channel)
{

    uint8_t i;

    for (i = 0; i < (uint8_t)PSU_CHANNEL_NUM; i++)
    {
        init_channel(&channel[i], (e_psu_channel)i);
    }

}

static void init_io(void)
{

    cli();

    /* UART */
    uart_init();
    stdout = &uart_output;
    stdin  = &uart_input;

    /* ADC */
    adc_init();

    /* PWM */
    pwm_init();

    /* System timer */
    timer_init();

    sei();

}

static void adc_processing(t_psu_channel *channel)
{

    /* Voltage */
    channel->voltage_readout.value.raw = adc_get(channel->voltage_adc_channel);
    lib_scale(&channel->voltage_readout.value, &channel->voltage_readout.scale);

    /* Current */
    channel->current_readout.value.raw = adc_get(channel->current_adc_channel);
    lib_scale(&channel->current_readout.value, &channel->current_readout.scale);

}

static void pwm_processing(t_psu_channel *channel)
{

    /* Voltage */
    lib_scale(&channel->voltage_setpoint.value, &channel->voltage_setpoint.scale);
    pwm_set_duty(channel->voltage_pwm_channel, channel->voltage_setpoint.value.scaled);

    /* Current */
    lib_scale(&channel->current_setpoint.value, &channel->current_setpoint.scale);
    pwm_set_duty(channel->current_pwm_channel, channel->current_setpoint.value.scaled);

}

static void input_processing(void)
{

    uint8_t i;

    for (i = 0; i < (uint8_t)PSU_CHANNEL_NUM; i++)
    {
        adc_processing(&psu_channels[i]);
    }

}

static void output_processing(void)
{
    uint8_t i;

    for (i = 0; i < (uint8_t)PSU_CHANNEL_NUM; i++)
    {
        pwm_processing(&psu_channels[i]);
    }

}

int main(void)
{

    /* System init */
    system_init();

    /* Initialize I/Os */
    init_io();

    /* Init ranges and precisions */
    init_psu(psu_channels);

/*
    channels[0].voltage_readout.value.raw = 250;
    lib_scale(&channels[0].voltage_readout.value, &channels[0].voltage_readout.scale);
    printf("Raw is %d and scaled is %d\r\n", channels[0].voltage_readout.value.raw, channels[0].voltage_readout.value.scaled);

    channels[0].voltage_readout.value.raw = 500;
    lib_scale(&channels[0].voltage_readout.value, &channels[0].voltage_readout.scale);
    printf("Raw is %d and scaled is %d\r\n", channels[0].voltage_readout.value.raw, channels[0].voltage_readout.value.scaled);

    channels[0].voltage_readout.value.raw = 750;
    lib_scale(&channels[0].voltage_readout.value, &channels[0].voltage_readout.scale);
    printf("Raw is %d and scaled is %d\r\n", channels[0].voltage_readout.value.raw, channels[0].voltage_readout.value.scaled);
*/

    printf("Starting the main loop\r\n");

    while (1)
    {
        //DBG_LOW;
        /* Periodic functions */
        //adc_periodic();
        input_processing();
        //printf("%d (%d)\r\n", channels[PSU_CHANNEL_0].voltage_readout.value.scaled, channels[PSU_CHANNEL_0].voltage_readout.value.raw);
        //_delay_ms(500);
        /** DEBUG PERIODIC FUNCS **/
        psu_channels[0].voltage_setpoint.value.raw= 11937; // observed an offset error of about 50mV
        // note that the prototype breadboard does not have a separatly filtered and regulated 5V supply
        psu_channels[0].current_setpoint.value.raw = 2047; // observed an offset error of about 40mV

        // to-do / to analyze: 1) absolute offset calibration
        //                     2) non linear behaviour correction (do measurements)

        /* Debug the timer */
        timer_debug();

        /* Output processing */
        output_processing();

    }

}
