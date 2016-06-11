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
 * @file pwm.c
 * @author Lorenzo Miori
 * @date Apr 2016
 * @brief PWM subsystem: initialization and routines to set the duty cycle
 */

#include "pwm.h"

#define PWM_FREQ        0x03FFU  /**< determines PWM frequency (15.6 kHz) */
#define PWM_MODE        1U       /* Fast (1) or Phase Correct (0) */

static t_pwm_channel pwm_channels[PWM_CHANNEL_NUM];

void pwm_init(void)
{

    /* only stub values */
    
    /* PWM_0 and PWM_1: 10 bit resolution, Fast PWM */

    pwm_channels[PWM_CHANNEL_0].resolution = PWM_FREQ;
    pwm_channels[PWM_CHANNEL_1].resolution = PWM_FREQ;

    /* PWM_2 and PWM_3: 8-bit resolution */

    pwm_channels[PWM_CHANNEL_2].resolution = 0xFF;
    pwm_channels[PWM_CHANNEL_3].resolution = 0xFF;

}

void pwm_enable_channel(e_pwm_channel pwm_channel)
{
    /* do nothing */
}

void pwm_set_duty(e_pwm_channel pwm_channel, uint16_t duty)
{
    /* do nothing */
}

uint16_t pwm_get_resolution(e_pwm_channel channel)
{
    return pwm_channels[channel].resolution;
}
