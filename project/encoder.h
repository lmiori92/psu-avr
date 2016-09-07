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
 * @file encoder.h
 * @author Lorenzo Miori
 * @date May 2016
 * @brief Encoder subsystem: initialization and routines to handle them
 */

#ifndef ENCODER_H_
#define ENCODER_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    ENC_HW_0,
    ENC_HW_NUM
} e_enc_hw;

typedef enum
{
    ENC_EVT_NONE,
    ENC_EVT_RIGHT,
    ENC_EVT_LEFT,
    ENC_EVT_CLICK_DOWN,
    ENC_EVT_CLICK_UP,
    ENC_EVT_TIMEOUT,
    ENC_EVT_NUM
} e_enc_event;

/** Event callback function */
typedef void (*t_enc_cb)(e_enc_event event, uint32_t delta_t);

typedef struct
{

    uint8_t  pin_A;
    uint8_t  pin_B;
    uint8_t  pin_raw;
    int8_t   raw;
    int8_t   value;
    uint32_t tick;
    uint32_t delta_t;
    t_enc_cb evt_cb;

} t_encoder;

/* Functions */
void encoder_init(void);
void encoder_set_callback(e_enc_hw index, t_enc_cb event_cb);

#endif /* ENCODER_H_ */
