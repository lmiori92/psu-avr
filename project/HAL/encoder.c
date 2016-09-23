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

#include "inc/encoder.h"

#include "inc/system.h"

#define ENC_DDR     DDRD
#define ENC_PORT    PORTD
#define ENC_PIN     PIND

/** Encoder status */
static t_encoder g_encoder;

/** Encoder lookup table */
static const int8_t enc_lookup [] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
/* coarse encoder lookup table
 * static const int8_t enc_lookup[16] = { 0, 0, -1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, -1, 0, 0 }; */
#define ENC_LOOKUP_NUM      (sizeof(enc_lookup) / sizeof(enc_lookup[0]))
#define ENC_TIMEOUT         500UL            /* ~ms (to improve performance, us are divided by 1024) */
#define ENC_STEP_COUNT      2                   /* steps to generate an event */

/**
 * Encoder subsystem initialization
 */
void encoder_init(void)
{

    /* Logic initialization */
    g_encoder.pin_A = PIN6;
    g_encoder.pin_B = PIN7;
    g_encoder.tick = 0U;

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

void encoder_tick(uint8_t timeout_ticks)
{
    if (g_encoder.tick >= timeout_ticks)
    {
        /* timeout ! */
    }
    else
    {
        g_encoder.tick++;
    }
}

/**
 * Set the encoder event callback
 */
void encoder_set_callback(e_enc_hw index, t_enc_cb event_cb)
{
    g_encoder.evt_cb = event_cb;
}

ISR(PCINT0_vect)
{
    /* Generate the click event */
    if ((PINB >> PIN0) & 0x01U)
    {
        g_encoder.evt_cb(ENC_EVT_CLICK_DOWN, 0U);
    }
    else
    {
        g_encoder.evt_cb(ENC_EVT_CLICK_UP, 0U);
    }

}

/**
 * Encoder subsystem interrupt handler
 */
ISR(PCINT2_vect)
{

    e_enc_event evt;

    {
        /* Shift the old values */
        g_encoder.pin_raw <<= 2;
        /* Store the new values */
        g_encoder.pin_raw |= ((ENC_PIN >> g_encoder.pin_A) & 0x1U) | (((ENC_PIN >> g_encoder.pin_B) & 0x1U) << 1U);
        /* Increment value by the lookup table value */
        g_encoder.raw += enc_lookup[g_encoder.pin_raw & 0x0FU];

        /* Compute the time difference */
//        delta_t  = g_timestamp;
//        delta_t -= g_encoder.tick;
//        delta_t /= 1024;

//        if (delta_t >= ENC_TIMEOUT)
//        {
            /* Timeout (delta time can never exceed 65535) */
//            delta_t = ENC_TIMEOUT;
//            evt = ENC_EVT_TIMEOUT;
//            goto evt_trig;
//        }
        if (g_encoder.raw > 2)
        {
            evt = ENC_EVT_RIGHT;
            goto evt_trig;
        }
        else if (g_encoder.raw < -2)
        {
            evt = ENC_EVT_LEFT;
            goto evt_trig;
        }

        return;
    }

evt_trig:
    g_encoder.raw = 0;
    g_encoder.evt_cb(evt, g_encoder.tick);
    g_encoder.tick = 0;
}
