/*
 * settings.c
 *
 *  Created on: 28 lug 2016
 *      Author: lorenzo
 */

#include "settings.h"
#include "string.h"

#include "eeprom.h"

#define PERSISTENT_CRC_START    0U
#define PERSISTENT_DATA_START  20U

static t_setting setting_list[SETTING_NUM_SETTINGS];
static uint16_t storage_size;

void settings_init(void)
{

    (void)memset(&setting_list, 0U, sizeof(setting_list));

    setting_list[SETTING_VERSION].type = SETTING_TYPE_VAL1;
    setting_list[SETTING_CAL_CURRENT_ZERO].type = SETTING_TYPE_VAL2;
    setting_list[SETTING_CAL_VOLTAGE_ZERO].type = SETTING_TYPE_VAL2;
    setting_list[SETTING_CAL_CURRENT_TOP].type = SETTING_TYPE_VAL2;
    setting_list[SETTING_CAL_VOLTAGE_TOP].type = SETTING_TYPE_VAL2;
    setting_list[SETTING_CAL_CURRENT_ZERO_SCALE].type = SETTING_TYPE_VAL2;
    setting_list[SETTING_CAL_VOLTAGE_ZERO_SCALE].type = SETTING_TYPE_VAL2;
    setting_list[SETTING_CAL_CURRENT_TOP_SCALE].type = SETTING_TYPE_VAL2;
    setting_list[SETTING_CAL_VOLTAGE_TOP_SCALE].type = SETTING_TYPE_VAL2;

    setting_set_1(SETTING_VERSION, 1U);
    setting_set_2(SETTING_CAL_CURRENT_ZERO, 0U);
    setting_set_2(SETTING_CAL_VOLTAGE_ZERO, 0U);
    setting_set_2(SETTING_CAL_CURRENT_TOP, 0U);
    setting_set_2(SETTING_CAL_VOLTAGE_TOP, 0U);
    setting_set_2(SETTING_CAL_CURRENT_ZERO_SCALE, 0U);
    setting_set_2(SETTING_CAL_VOLTAGE_ZERO_SCALE, 0U);
    setting_set_2(SETTING_CAL_CURRENT_TOP_SCALE, 0U);
    setting_set_2(SETTING_CAL_VOLTAGE_TOP_SCALE, 0U);

    /* initialize the persistent storage layer */
    persistent_init(&storage_size);

}

bool setting_is_changed(e_settings_available setting)
{
    if ((uint8_t)setting < (uint8_t)SETTING_NUM_SETTINGS)
    {
        return setting_list[setting].changed;
    }
    else
    {
        return false;
    }
}

void setting_set_changed(e_settings_available setting, bool changed)
{
    if ((uint8_t)setting < (uint8_t)SETTING_NUM_SETTINGS)
    {
        setting_list[setting].changed = true;
    }
    else
    {
        /* not valid */
    }
}

static void setting_save_to_storage(e_settings_available setting)
{
    if (setting_list[setting].changed == true)
    {
        /* changed, save it */
        persistent_write(0U + PERSISTENT_DATA_START + ((uint16_t)setting * 4U), setting_list[setting].u_setting_value.byte4 & 0xFFU);
        persistent_write(1U + PERSISTENT_DATA_START + ((uint16_t)setting * 4U), (setting_list[setting].u_setting_value.byte4 >> 8U) & 0xFFU);
        persistent_write(2U + PERSISTENT_DATA_START + ((uint16_t)setting * 4U), (setting_list[setting].u_setting_value.byte4 >> 16U) & 0xFFU);
        persistent_write(3U + PERSISTENT_DATA_START + ((uint16_t)setting * 4U), (setting_list[setting].u_setting_value.byte4 >> 24U) & 0xFFU);
    }
    else
    {
        /* not changed, no need to save */
    }

}

void settings_read_from_storage(void)
{
    // placeholder
}

void settings_save_to_storage(e_settings_available setting)
{
    uint16_t param;

    if (setting == SETTING_NUM_SETTINGS)
    {
        /* Save ALL */
        for (param = 0U; param < (uint16_t)SETTING_NUM_SETTINGS; param++)
        {
            setting_save_to_storage((e_settings_available)param);
        }
    }
    else
    {
        /* Save specific setting */
        setting_save_to_storage(setting);
    }
}

static bool setting_valid_type(e_settings_available setting, e_setting_type type)
{
    if ((uint8_t)setting < (uint8_t)SETTING_NUM_SETTINGS)
    {
        return (setting_list[setting].type == type) ? true : false;
    }
    else
    {
        return false;
    }
}

uint8_t setting_get_1(e_settings_available setting, uint8_t default_val)
{
    bool valid;
    valid = setting_valid_type(setting, SETTING_TYPE_VAL1);

    if (valid == true) return setting_list[setting].u_setting_value.byte1;
    else return default_val;
}
uint16_t setting_get_2(e_settings_available setting, uint16_t default_val)
{
    bool valid;
    valid = setting_valid_type(setting, SETTING_TYPE_VAL2);

    if (valid == true) return setting_list[setting].u_setting_value.byte2;
    else return default_val;
}
uint32_t setting_get_4(e_settings_available setting, uint32_t default_val)
{
    bool valid;
    valid = setting_valid_type(setting, SETTING_TYPE_VAL4);

    if (valid == true) return setting_list[setting].u_setting_value.byte4;
    else return default_val;
}
bool setting_get_bool(e_settings_available setting, bool default_val)
{
    bool valid;
    valid = setting_valid_type(setting, SETTING_TYPE_BOOL);

    if (valid == true) return setting_list[setting].u_setting_value.byteB;
    else return default_val;
}

void setting_set_1(e_settings_available setting, uint8_t value)
{
    bool valid;
    valid = setting_valid_type(setting, SETTING_TYPE_VAL1);

    if (valid == true)
    {
        setting_list[setting].u_setting_value.byte1 = value;
        setting_list[setting].changed = true;
    }
}
void setting_set_2(e_settings_available setting, uint16_t value)
{
    bool valid;
    valid = setting_valid_type(setting, SETTING_TYPE_VAL2);

    if (valid == true)
    {
        setting_list[setting].u_setting_value.byte2 = value;
        setting_list[setting].changed = true;
    }
}
void setting_set_4(e_settings_available setting, uint32_t value)
{
    bool valid;
    valid = setting_valid_type(setting, SETTING_TYPE_VAL4);

    if (valid == true)
    {
        setting_list[setting].u_setting_value.byte4 = value;
        setting_list[setting].changed = true;
    }
}
void setting_set_bool(e_settings_available setting, bool value)
{
    bool valid;
    valid = setting_valid_type(setting, SETTING_TYPE_BOOL);

    if (valid == true)
    {
        setting_list[setting].u_setting_value.byteB = value;
        setting_list[setting].changed = true;
    }
}
