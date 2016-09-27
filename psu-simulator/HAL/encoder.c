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

#include "ncurses.h"

#define __USE_GNU    /* pthread_yield() */
#include "pthread.h"

/** Encoder status */
static t_encoder g_encoder;

/** Input thread  */
static pthread_t keyboard_thread;

/* Very crude but effective encoder simulator */
void* keyboard_thread_worker(void *params)
{
    int key;

    (void)params;

    g_encoder.tick = 24;

    while(1)
    {
        key = wgetch(stdscr);
        switch(key)
        {
        case KEY_LEFT:
            g_encoder.raw = 0;
            g_encoder.evt_cb(ENC_EVT_LEFT, g_encoder.tick);
            break;
        case KEY_RIGHT:
            g_encoder.evt_cb(ENC_EVT_RIGHT, g_encoder.tick);
            break;
        case KEY_UP:
            g_encoder.evt_cb(ENC_EVT_CLICK_DOWN, g_encoder.tick);
            break;
        case KEY_DOWN:
            g_encoder.evt_cb(ENC_EVT_CLICK_UP, g_encoder.tick);
            break;
        default:
            break;
        }

    }
}

/**
 * Encoder subsystem initialization
 */
void encoder_init(void)
{
    /* keyboard on linux via ncurses */

    /* important attributes */
    noecho();
    cbreak();   /* Line buffering disabled. pass on everything */
    keypad(stdscr, TRUE);   /* enable keyboard input */
    nodelay(stdscr,0);

    /* Start the input thread */
    pthread_create(&keyboard_thread, NULL, keyboard_thread_worker, NULL);
}

/**
 * Set the encoder event callback
 */
void encoder_set_callback(e_enc_hw index, t_enc_cb event_cb)
{
    g_encoder.evt_cb = event_cb;
}
