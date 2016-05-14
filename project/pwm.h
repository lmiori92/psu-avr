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
 * @file pwm.h
 * @author Lorenzo Miori
 * @date Apr 2016
 * @brief PWM subsystem: initialization and routines to set the duty cycle
 */

#ifndef PWM_H_
#define PWM_H_

typedef enum
{
    PWM_CHANNEL_0,
    PWM_CHANNEL_1,
    PWM_CHANNEL_2,
    PWM_CHANNEL_3,

    PWM_CHANNEL_NUM
} e_pwm_channel;

typedef struct
{
    uint16_t      resolution;
} t_pwm_channel;

void pwm_init(void);
void pwm_enable_channel(e_pwm_channel pwm_channel);
void pwm_set_duty(e_pwm_channel pwm_channel, uint16_t duty);
uint16_t pwm_get_resolution(e_pwm_channel channel);

#endif /* PWM_H_ */
