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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "adc.h"

#include "time_m.h"

/* Quick noise debug */
#ifdef ADC_NOISE_DEBUG
uint16_t adc_maxS = 0;
uint16_t adc_minS = 0xFFFF;
uint16_t last_captureS = 0;           /**< Last reading */
#endif

/* Timing debug (o-scope) */
#ifdef ADC_SCOPE_DEBUG
#define DBG_LOW      PORTD &= ~(1 << PIN2);asm("nop")  /**< DBG LOW */
#define DBG_HIGH     PORTD |=  (1 << PIN2);asm("nop")  /**< DBG HIGH */
#define DBG_CONFIG   DDRD |= (1 << PIN2)
#endif

static uint8_t  adc_mux_index;
static bool     adc_mux_switch;
static uint16_t adc_samples[ADC_NUM];

/**
 * ISR(ADC_vect)
 *
 * @brief ADC interrupt routine.
 *        Keep it as small as possible to avoid jitter and lower
 *        sampling rate.
 *
 */
ISR(ADC_vect)
{

#ifdef ADC_SCOPE_DEBUG
    DBG_LOW;
#endif

    /* Save capture in the buffer */
#ifdef ADC_NOISE_DEBUG
    last_captureS = (ADCL | (ADCH << 8U));
    capture[capture_index] = last_captureS;
    /* HARDWARE NOISE DEBUG */
    if (last_captureS > adc_maxS) adc_maxS = last_captureS;
    if (last_captureS < adc_minS) adc_minS = last_captureS;
#else
    if (adc_mux_switch == false)
    {
        /* Store the value */
        adc_samples[adc_mux_index] = (ADCL | (ADCH << 8U));
        /* Advance the MUX index */
        adc_mux_index = (adc_mux_index + 1) % ADC_NUM;
        /* Update the ADC MUX register */
        ADMUX &= ~((1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0));
        ADMUX |= adc_mux_index;
        adc_mux_switch = true;
    }
    else
    {
        /* creates jitter, but we don't care here */
        adc_mux_switch = false;
    }
#endif

#ifdef ADC_SCOPE_DEBUG
    DBG_HIGH;
#endif

    /* Kick-in another conversion */
    /* Set ADSC in ADCSRA (0x7A) to start another ADC conversion */
    ADCSRA |= (1 << ADSC);

}

/**
 *
 * ma_audio_init
 *
 * @brief ADC initialization, assuming the interrupts are disabled.
 *
 */
void adc_init(void)
{

    /* clear ADLAR in ADMUX (0x7C) to right-adjust the result */
    /* ADCL will contain lower 8 bits, ADCH upper 2 (in last two bits) */
    /* Set REFS1..0 in ADMUX to change reference voltage */
    /* Clear MUX3..0 in ADMUX (0x7C) in preparation for setting the analog channel */
    ADMUX &= ~((1 << ADLAR) | (1 << REFS1) | (1 << REFS0) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0));

    /* Set the AVcc reference */
    ADMUX |= ((0 << REFS1) | (1 << REFS0));

    /* Set ADEN in ADCSRA (0x7A) to enable the ADC.
       Note, this instruction takes 12 ADC clocks to execute
     */
    ADCSRA |= (1 << ADEN);

    /* Disable Free Running Mode (we want to change channel in the ISR) */
    //ADCSRA &= ~(1 << ADFR);

    /* Set the Prescaler to 64 */
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (0 << ADPS0) |
              /* Set ADIE in ADCSRA (0x7A) to enable the ADC interrupt. */
              (1 << ADIE) |
              /* Kickstart the process */
              (1 << ADSC);

#ifdef ADC_SCOPE_DEBUG
    DBG_CONFIG;
#endif

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


/* TODO */
/*
#include <avr/delay.h>
long readVcc() {

  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif

  _delay_ms(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}
*/

#endif /* _ADC_H_ */

