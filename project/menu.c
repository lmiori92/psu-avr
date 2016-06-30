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
 * @file menu.c
 * @author Lorenzo Miori
 * @date Jun 2016
 * @brief MENU: handling of simple menu logic for character displays
 */


#include "display.h"    /* display primitives */
#include "menu.h"       /* menu definitions */

static char* BOOL_LABELS[2] = { "NO", "YES" };
static uint8_t BOOL_VALUES[2] = { (uint8_t)false, (uint8_t)true };

/** Static declarations **/
static void menu_extra_display(t_menu_item *item);
static void menu_index_edit(t_menu_state *state, uint8_t count, bool increment);
static void menu_extra_edit(t_menu_item *item, bool increment);

void menu_init_bool_list(t_menu_extra_list *extra)
{
    extra->count = 2U;
    extra->labels = BOOL_LABELS;
    extra->values = BOOL_VALUES;
}

void menu_init(t_menu_state *state, t_menu_item *item, uint8_t count)
{
    uint8_t i;
    for (i = 0; i < count; i++)
    {
        /* item not selected initially */
        item->state = MENU_NOT_SELECTED;
    }
    state->index = 0;
    state->index_prev = 0;
}

static void menu_extra_display(t_menu_item *item)
{

    uint8_t u8_tmp;
    uint8_t u16_tmp;
    t_menu_extra_list *ext_ptr_tmp;

    if ((item->extra != NULL) && (item->extra->ptr != NULL))
    {
        /* dereference and operate the pointer based on the defined type */
        switch(item->extra->type)
        {
            case MENU_TYPE_LIST:
                /* 8-bit array index */
                /* display the "enum-like" element */
                ext_ptr_tmp = (t_menu_extra_list*)(item->extra->ptr);
                u8_tmp = *(ext_ptr_tmp->ptr);
                display_write_string(ext_ptr_tmp->labels[u8_tmp]);
                break;
            case MENU_TYPE_NUMERIC_8:
                /* 8-bit number */
                u8_tmp = (*((uint8_t*)item->extra->ptr));
                display_write_number((uint16_t)u8_tmp, false);
                break;
            case MENU_TYPE_NUMERIC_16:
                /* 16-bit number */
                u16_tmp = (*((uint16_t*)item->extra->ptr));
                display_write_number(u16_tmp, false);
                break;
            default:
                /* not implemented */
                break;
        }
    }
}

static void menu_index_edit(t_menu_state *state, uint8_t count, bool increment)
{
    if (count > 0)
    {
        if (increment == true)
        {
            /* increment - "menu down" */
            if (state->index < (count - 1))
            {
                state->index_prev = state->index;  /* previous value */
                state->index++;                    /* increment value */
            }
        }
        else
        {
            /* decrement - "menu up" */
            if (state->index > 1U)
            {
                state->index_prev = state->index;  /* previous value */
                state->index--;                    /* increment value */
            }
        }
    }
}

static void menu_extra_edit(t_menu_item *item, bool increment)
{
    t_menu_extra_list* tmp_extra;
    if ((item->extra != NULL) && (item->extra->ptr != NULL))
    {
        /* dereference and operate the pointer based on the defined type */
        switch(item->extra->type)
        {
            case MENU_TYPE_LIST:
                /* 8-bit number for indexing the type "list" */
                tmp_extra = (t_menu_extra_list*)item->extra->ptr;
                if (increment == true) *tmp_extra->ptr = (*(tmp_extra->ptr)) + 1U;
                else *tmp_extra->ptr = (*(tmp_extra->ptr)) - 1U;
                break;
            case MENU_TYPE_NUMERIC_8:
                /* 8-bit number */
                if (increment == true) (*((uint8_t*)item->extra->ptr))++;
                else (*((uint8_t*)item->extra->ptr))--;
                break;
            case MENU_TYPE_NUMERIC_16:
                /* 16-bit number */
                if (increment == true) (*((uint16_t*)item->extra->ptr))++;
                else (*((uint16_t*)item->extra->ptr))--;
                break;
            default:
                /* not implemented */
                break;
        }
    }
}

/**
This routine displays the current state of the menu i.e. labels, selection
and possibly extra data like ON-OFF labels or numeric values.
*/
void menu_display(t_menu_state *state, t_menu_item *item, uint8_t count)
{

    t_menu_item *item_ptr;

    /* for faster execution time, work with the pointer of the items */
    item_ptr = item;    

    /* selected item arrow display */
    display_set_cursor(state->index % 2, 0);
    display_write_char(0x7EU);

    /* display labels and extra data */

    display_set_cursor(0, 1);                                       /* cursor right after the (possible) arrow */
    display_write_string((item_ptr + state->index)->label);         /* label */
    display_set_cursor(0, 2);
    menu_extra_display(item);
    display_set_cursor(0, 1);                                       /* cursor right after the (possible) arrow */
    display_write_string((item_ptr + state->index_prev)->label);    /* label */
    display_set_cursor(0, 2);
    menu_extra_display(item);

}

void menu_event(e_menu_event event, t_menu_state *state, t_menu_item *item, uint8_t count)
{
    switch(event)
    {
        case MENU_EVENT_NONE:
            /* no event; NOOP */
            break;
        case MENU_EVENT_CLICK:
            /* if a simple entry, do nothing (probably changing menu anyhow) */
            if (item->state == MENU_NOT_SELECTED) item->state = MENU_SELECTED;
            else item->state = MENU_SELECTED;
            break;
        case MENU_EVENT_CLICK_LONG:
            /* NOOP for the moment */
            break;
        case MENU_EVENT_LEFT:
            if (item->state == MENU_SELECTED) menu_extra_edit(item, false);
            else menu_index_edit(state, count, false);
            break;
        case MENU_EVENT_RIGHT:
            if (item->state == MENU_SELECTED) menu_extra_edit(item, true);
            else menu_index_edit(state, count, true);
            break;
        default:
            /* no event; NOOP */
            break;
    }
}
