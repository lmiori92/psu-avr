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

#ifndef ADC_H_
#define ADC_H_

/*#define ADC_NOISE_DEBUG*/
/*#define ADC_SCOPE_DEBUG*/

#include <stdint.h>

#define ADC_RESOLUTION      1023U       // TODO get it via an ADC API

typedef enum _e_adc_channels
{
    ADC_0,
    ADC_1,
    ADC_2,
    ADC_3,
    ADC_4,
    ADC_5,

    ADC_NUM
} e_adc_channel;

void adc_init(void);
uint16_t adc_get(e_adc_channel channel);
void adc_last_capture(uint16_t *last_capture, uint16_t *adc_min, uint16_t *adc_max);
void adc_last_reset(void);

#ifdef ADC_SCOPE_DEBUG
void adc_periodic(void);
#endif

#endif /* ADC_H_ */
