
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

#ifndef _KEYPAD_H_
#define _KEYPAD_H_

#include <stdint.h>
#include <stdbool.h>

#define DEBOUNCE_BUTTONS    20000       /**< Debounce time [us] */

/**< Enumeration of buttons */
typedef enum e_buttons_
{
    BUTTON_SELECT,

    NUM_BUTTONS
} e_key;

typedef struct
{

    bool       input[NUM_BUTTONS];
    uint32_t   debounce[NUM_BUTTONS];
    bool       latches[NUM_BUTTONS];
    bool       buttons[NUM_BUTTONS];

} t_keypad;

void keypad_init(void);
bool keypad_clicked(e_key key);

void keypad_set_input(e_key key, bool value);
void keypad_periodic(uint32_t timestamp);

#endif /* _KEYPAD_H_ */
