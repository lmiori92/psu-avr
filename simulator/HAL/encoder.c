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

#include <stdint.h>

#include "encoder.h"
#include "system.h"
#include "time.h"

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
    /* do nothing on linux */
}

/**
 * Set the encoder event callback
 */
void encoder_set_callback(e_enc_hw index, t_enc_cb event_cb)
{
    g_encoder[index].evt_cb = event_cb;
}
