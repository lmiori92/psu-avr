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

#include <avr/io.h>
#include <avr/interrupt.h>

#include "pwm.h"

#define PWM_FREQ        0x03FFU  /**< determines PWM frequency (15.6 kHz) */
#define PWM_MODE        1U       /* Fast (1) or Phase Correct (0) */

static t_pwm_channel pwm_channels[PWM_CHANNEL_NUM];

static void pwm_config_channel(e_pwm_channel pwm_channel)
{
    /* Enable output and set output pins */

    switch(pwm_channel)
    {
    case PWM_CHANNEL_0:
        TCCR1A |= (1 << COM1A1);
        DDRB   |= (1 << PIN1);
        break;
    case PWM_CHANNEL_1:
        TCCR1A |= (1 << COM1B1);
        DDRB   |= (1 << PIN2);
        break;
    case PWM_CHANNEL_2:
        TCCR2A |= (1 << COM2A1);
        DDRB   |= (1 << PIN3);
        break;
    case PWM_CHANNEL_3:
        TCCR2A |= (1 << COM2B1);
        DDRD   |= (1 << PIN3);
        break;
    default:
        break;
    }
}

void pwm_init(void)
{

    /* PWM_0 and PWM_1: 10 bit resolution, Fast PWM */
    TCCR1A = (PWM_MODE << 1);
    TCCR1B = ((PWM_MODE << 3) | (1 << WGM13) | (1 << CS10));
    ICR1H = (PWM_FREQ >> 8);
    ICR1L = (PWM_FREQ & 0xff);

    pwm_channels[PWM_CHANNEL_0].resolution = PWM_FREQ;
    pwm_channels[PWM_CHANNEL_1].resolution = PWM_FREQ;

    /* PWM_2 and PWM_3: 8-bit resolution */

    /* PWM Phase Correct and Clear OCR2x on match */
    TCCR2A = (1 << WGM20);
    /* set up timer with prescaler */
    TCCR2B = (0 << WGM22) | (1 << CS20);

    pwm_channels[PWM_CHANNEL_2].resolution = 0xFF;
    pwm_channels[PWM_CHANNEL_3].resolution = 0xFF;

}

void pwm_enable_channel(e_pwm_channel pwm_channel)
{
    pwm_config_channel(pwm_channel);
}

void pwm_set_duty(e_pwm_channel pwm_channel, uint16_t duty)
{

    switch(pwm_channel)
    {
    case PWM_CHANNEL_0:
        OCR1AH = duty >> 8;
        OCR1AL = duty;
        break;
    case PWM_CHANNEL_1:
        OCR1BH = duty >> 8;
        OCR1BL = duty;
        break;
    case PWM_CHANNEL_2:
        OCR2A = duty;
        break;
    case PWM_CHANNEL_3:
        OCR2B = duty;
        break;
    default:
        break;
    }
}

uint16_t pwm_get_resolution(e_pwm_channel channel)
{
    return pwm_channels[channel].resolution;
}
