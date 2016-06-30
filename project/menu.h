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
 * @file menu.h
 * @author Lorenzo Miori
 * @date Apr 2016
 * @brief MENU: handling of simple menu logic for character displays
 */

#ifndef MENU_H_
#define MENU_H_

#include <stdint.h>

/** Enumeration of supported events the menu system can handle */
typedef enum
{
    MENU_EVENT_NONE,
    MENU_EVENT_CLICK,
    MENU_EVENT_CLICK_LONG,
    MENU_EVENT_LEFT,
    MENU_EVENT_RIGHT
} e_menu_event;

/** Enumeration of supported menu item types */
typedef enum
{
    MENU_TYPE_LIST,            /**< Menu item extra type is a list */
    MENU_TYPE_NUMERIC_8,       /**<  */
    MENU_TYPE_NUMERIC_16       /**<  */
} e_menu_type;

/** Enumeration of the possible states a menu item can be */
typedef enum
{
    MENU_NOT_SELECTED,         /**< Menu item is not selected to perform actions on it */
    MENU_SELECTED              /**< Menu item is selected and actions can be performed on it */
} e_menu_item_state;

/** Structure for the extra LIST item information (labels and values) */
typedef struct
{
    uint8_t  count;           /**< Total number of labels */
    uint8_t  *ptr;            /**< The index of the currently active label */
    char*    *labels;         /**< Labels to associate the ptr value to */
    uint8_t  *values;         /**< List of values to associate the ptr to */
} t_menu_extra_list;

/** Structure for the extra item information */
typedef struct
{
    void*           ptr;       /**< Pointer to the modifiable value */
    e_menu_type     type;      /**< Menu type; determines extra field type */
} t_menu_extra;

/** Structure for describing the menu item */
typedef struct
{
    char*               label;  /**< Item has this string as label */
    t_menu_extra*       extra;  /**< When != NULL an extra is appended, depending on the type of menu */
    e_menu_item_state   state;  /**< Item state */
} t_menu_item;

/** Structure for the current menu state */
typedef struct
{
    uint8_t index;              /**< Selected menu item */
    uint8_t index_prev;         /**< Previously selected menu item */
} t_menu_state;

/**  EXTERNAL LINKAGE FUNCTIONS **/

void menu_init(t_menu_state *state, t_menu_item *item, uint8_t count);
void menu_display(t_menu_state *state, t_menu_item *item, uint8_t count);
void menu_event(e_menu_event event, t_menu_state *state, t_menu_item *item, uint8_t count);
void menu_init_bool_list(t_menu_extra_list *extra);

#endif /* MENU_H_ */
