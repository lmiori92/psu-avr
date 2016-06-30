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

#include "keypad.h"

#include <stdbool.h>

static t_keypad keypad;
static uint32_t cur_timestamp = 0;

void keypad_init(void)
{
    uint8_t i = 0;

    /* Init keypad */
    for (i = 0; i < NUM_BUTTONS; i++)
    {
        keypad.input[i] = false;
        keypad.buttons[i]  = false;
        keypad.latches[i]  = false;
        keypad.debounce[i] = 0;
    }
}

void keypad_set_input(e_key key, bool value)
{
    keypad.input[key] = value;
}

e_key_event keypad_clicked(e_key key)
{
    return keypad.buttons[key];
}

/* Read the keypad, apply debounce to inputs and detect the rising edge */
void keypad_periodic(uint32_t timestamp)
{

    uint8_t i = 0;
    bool flag_50ms = false;

    /* check 50ms flag */
    if ((timestamp - cur_timestamp) > KEY_50MS_FLAG)
    {
        /* 50ms elapsed, set the flag */
        flag_50ms = true;
        cur_timestamp = timestamp;
    }
    else
    {
        /* waiting for the 50ms flag */
    }

    for (i = 0; i < NUM_BUTTONS; i++)
    {
        /* the event shall be consumed within a cycle */
        keypad.buttons[i] = KEY_NONE;

        /* debounce the raw input */
        if ((flag_50ms == true) && (keypad.debounce[i] < KEY_DEBOUNCE_HOLD))
        {
            /* flag available, count up */
            keypad.debounce[i]++;
        }
        else
        {
            /* flag not available, wait next round */
        }

        if (keypad.latches[i] == false && keypad.input[i] == true)
        {
            /* rising edge */
            keypad.debounce[i] = 0;
        }
        else if (keypad.latches[i] == true && keypad.input[i] == true)
        {

            if (keypad.debounce[i] == KEY_DEBOUNCE_HOLD)
            {
                /* button stayed high for long enough for a hold event */
                keypad.buttons[i] = KEY_HOLD;
                /* prevent overflow */
                keypad.debounce[i] = KEY_DEBOUNCE_HOLD + 1U;
            }
            else
            {
                /* not yet triggered or timeout */
            }
        }
        else
        {
            if (keypad.debounce[i] > KEY_DEBOUNCE_CLICK && keypad.debounce[i] < KEY_DEBOUNCE_HOLD)
            {
                /* click event */
                keypad.buttons[i] = KEY_CLICK;
            }
            else
            {
                /* either not enough for a click or hold event */
            }

            keypad.debounce[i] = 0;
        }

        keypad.latches[i] = keypad.input[i];

    }

}
