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
 * @file adc.c
 * @author Lorenzo Miori
 * @date Apr 2016
 * @brief The ADC processing unit.
 */

#ifndef _ADC_H_
#define _ADC_H_

#include <stdbool.h>
#include "adc.h"
#include "time.h"

static uint8_t  adc_mux_index;
static bool     adc_mux_switch;
static uint16_t adc_samples[ADC_NUM];

/**
 *
 * ma_audio_init
 *
 * @brief ADC initialization, assuming the interrupts are disabled.
 *
 */
void adc_init(void)
{
    /* do nothing on linux */
}

#ifdef ADC_SCOPE_DEBUG
void adc_periodic(void)
{
    /* STUB */
    static uint32_t ts = 0;
    uint8_t i = 0;
    if (g_timestamp > (ts + 1000000))
    {
        ts = g_timestamp;
        for (i = 0; i < ADC_NUM; i++)
        {
            printf("%d: %d\r\n", i, adc_samples[i]);
        }
    }
}
#else
/* nothing */
#endif

uint16_t adc_get(e_adc_channel channel)
{
    return adc_samples[channel];
}

/**
 *
 * ma_audio_last_capture
 *
 * @brief Getter function for the last ADC capture
 *
 * @param   last_capture    pointer to the variable to store the last capture value
 * @param   adc_min         pointer to the variable to store the minimum value
 * @param   adc_max         pointer to the variable to store the maximum value
 * @return the value for the last capture
 *
 */
void adc_last_capture(uint16_t *last_capture, uint16_t *adc_min, uint16_t *adc_max)
{
#ifdef ADC_NOISE_DEBUG
    *last_capture = last_captureS;
    *adc_min = adc_minS;
    *adc_max = adc_maxS;
#endif
}

void adc_last_reset(void)
{
#ifdef ADC_NOISE_DEBUG
    last_captureS = 0;
    adc_minS = 0xFFFF;
    adc_maxS = 0;
#endif
}

#endif /* _ADC_H_ */

