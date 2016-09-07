/*
 * settings.h
 *
 *  Created on: 28 lug 2016
 *      Author: lorenzo
 */

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "stdint.h"
#include "stdbool.h"

typedef enum
{
    SETTING_STATE_CHANGED = 0x01U,
    SETTING_STATE_VALID   = 0x02U
} e_setting_property;

typedef enum
{
    SETTING_VERSION,

//    SETTING_CAL_CURRENT_ZERO,
//    SETTING_CAL_VOLTAGE_ZERO,
//    SETTING_CAL_CURRENT_TOP,
//    SETTING_CAL_VOLTAGE_TOP,

//    SETTING_CAL_CURRENT_ZERO_SCALE,
//    SETTING_CAL_VOLTAGE_ZERO_SCALE,
//    SETTING_CAL_CURRENT_TOP_SCALE,
//    SETTING_CAL_VOLTAGE_TOP_SCALE,

    SETTING_NUM_SETTINGS
} e_settings_available;

typedef enum
{
    SETTING_UNDEFINED,

    SETTING_TYPE_VAL1,
    SETTING_TYPE_VAL2,
    SETTING_TYPE_VAL4,

    SETTING_TYPE_BOOL
} e_setting_type;

typedef struct
{
    e_setting_type type;
    bool           state;

    union
    {
        uint8_t  byte1;
        uint16_t byte2;
        uint32_t byte4;
        bool     byteB;
        uint8_t  byte_array[4U];
    } u_setting_value;

} t_setting;

void settings_init(void);

bool setting_has_property(e_settings_available setting, e_setting_property property);
void setting_set_property(e_settings_available setting, e_setting_property property, bool state);

void settings_save_to_storage(e_settings_available setting);
void settings_read_from_storage(void);


uint8_t setting_get_1(e_settings_available setting, uint8_t default_val);
uint16_t setting_get_2(e_settings_available setting, uint16_t default_val);
uint32_t setting_get_4(e_settings_available setting, uint32_t default_val);
bool setting_get_bool(e_settings_available setting, bool default_val);

void setting_set_1(e_settings_available setting, uint8_t value);
void setting_set_2(e_settings_available setting, uint16_t value);
void setting_set_4(e_settings_available setting, uint32_t value);
void setting_set_bool(e_settings_available setting, bool value);

#endif /* SETTINGS_H_ */
