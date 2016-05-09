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
 * @file encoder.c
 * @author Lorenzo Miori
 * @date May 2016
 * @brief Encoder subsystem: initialization and routines to handle them
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "encoder.h"
#include "system.h"
#include "time.h"

#define ENC_DDR     DDRD
#define ENC_PORT    PORTD
#define ENC_PIN     PIND

/** Encoder status */
static t_encoder g_encoder[ENC_HW_NUM];

/** Encoder lookup table */
static const int8_t enc_lookup [] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
/* coarse encoder lookup table
 * static const int8_t enc_lookup[16] = { 0, 0, -1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, -1, 0, 0 }; */
#define ENC_LOOKUP_NUM      (sizeof(enc_lookup) / sizeof(enc_lookup[0]))
#define ENC_TIMEOUT         500000UL            /* us */
#define ENC_STEP_COUNT      2                   /* steps to generate an event */

/**
 * Encoder subsystem initialization
 */
void encoder_init(void)
{

    /* Logic initialization */
    g_encoder[ENC_HW_0].pin_A = PIN6;
    g_encoder[ENC_HW_0].pin_B = PIN7;
    g_encoder[ENC_HW_0].tick = g_timestamp;

    /* Inputs */
    ENC_DDR &= ~(1<<PIN7);
    ENC_DDR &= ~(1<<PIN6);
    DDRB &= ~(1<<PIN0);

    /* Turn on pull-ups (encoder switches to GND) */
    ENC_PORT |= (1<<PIN7) | (1 << PIN6);
    PORTB |= 1<<PIN0;

    /* Enable interrupts on the encoder pins */
    PCMSK0 |= (1 << PCINT0 );                  /* click */
    PCMSK2 |= (1 << PCINT23 ) | (1 << PCINT22); /* wheel */

    /* Enable Pin Change subsystem (interrupts) */
    PCICR |= (1<< PCIE0);
    PCICR |= (1<< PCIE2);

}

/**
 * Set the encoder event callback
 */
void encoder_set_callback(e_enc_hw index, t_enc_cb event_cb)
{
    g_encoder[index].evt_cb = event_cb;
}

ISR(PCINT0_vect)
{
    /* TODO generate the appropriate event */
}

/**
 * Encoder subsystem interrupt handler
 * Best case execution time: 10us
 * Worst case execution time: 20us
 * => both well below the 100us system tick timer
 */
ISR(PCINT2_vect)
{

    uint8_t i = 0;  /* left for a future multiple encoder implementation */

    {
        /* Shift the old values */
        g_encoder[i].pin_raw <<= 2;
        /* Store the new values */
        g_encoder[i].pin_raw |= ((ENC_PIN >> g_encoder[i].pin_A) & 0x1U) | (((ENC_PIN >> g_encoder[i].pin_B) & 0x1U) << 1U);
        /* Increment value by the lookup table value */
        g_encoder[i].raw += enc_lookup[g_encoder[0].pin_raw & 0x0FU];
        if ((g_timestamp - g_encoder[i].tick) > ENC_TIMEOUT)
        {
            /* Timeout */
            g_encoder[i].raw = 0;
            g_encoder[i].pin_raw = 0;
            g_encoder[i].tick = g_timestamp;
        }
        else if (g_encoder[i].raw > 2)
        {
            g_encoder[i].delta_t = g_timestamp - g_encoder[i].tick;
            g_encoder[i].value++;
            g_encoder[i].raw = 0;
            g_encoder[i].tick = g_timestamp;
            g_encoder[i].evt_cb(ENC_EVT_RIGHT, g_encoder[i].delta_t);
        }
        else if (g_encoder[i].raw < -2)
        {
            g_encoder[i].delta_t = g_timestamp - g_encoder[i].tick;
            g_encoder[i].value--;
            g_encoder[i].raw = 0;
            g_encoder[i].tick = g_timestamp;
            g_encoder[i].evt_cb(ENC_EVT_LEFT, g_encoder[i].delta_t);
        }
    }

}
