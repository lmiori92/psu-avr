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

bool keypad_clicked(e_key key)
{
    return keypad.buttons[key];
}

/* Read the keypad, apply debounce to inputs and detect the rising edge */
void keypad_periodic(uint32_t timestamp)
{

  uint8_t i = 0;
  bool t = false;

  for (i = 0; i < NUM_BUTTONS; i++)
  {
      t = keypad.input[i];

      if (t == true)
      {
        t = false;

        /* debounce the raw input */
        if (keypad.debounce[i] == 0)
        {
            keypad.debounce[i] = timestamp;
        }
        else
        {
          if ((timestamp - keypad.debounce[i]) > DEBOUNCE_BUTTONS)
          {
              t = true;
          }
        }
      }
      else
      {
          keypad.debounce[i] = 0;
      }

      if (t == true && keypad.latches[i] == false)
      {
          /* Falling edge */
          keypad.buttons[i] = true;
      }
      else
      {
          keypad.buttons[i] = false;
      }

      keypad.latches[i] = t;
  }

}
