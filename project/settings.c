/*
 * settings.c
 *
 *  Created on: 28 lug 2016
 *      Author: lorenzo
 */

#include "settings.h"
#include "string.h"

#include "HAL/inc/eeprom.h"

/* External Library */
#include "lorenzlib/lib.h"     /* CRC16 CCITT */

#define PERSISTENT_CRC_START    0U
#define PERSISTENT_DATA_START  20U
#define PERSISTENT_CRC_SEED    0xFFFFU
static t_setting setting_list[SETTING_NUM_SETTINGS];
static uint16_t storage_size;

void settings_init(void)
{

    (void)memset(&setting_list, 0U, sizeof(setting_list));

    setting_list[SETTING_VERSION].type = SETTING_TYPE_VAL1;
    /*
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
    */

    /* initialize the persistent storage layer */
    persistent_init(&storage_size);

}

bool setting_has_property(e_settings_available setting, e_setting_property property)
{
    if ((uint8_t)setting < (uint8_t)SETTING_NUM_SETTINGS)
    {
        return ((setting_list[setting].state & (uint8_t)property) == (uint8_t)property);
    }
    else
    {
        return false;
    }
}

void setting_set_property(e_settings_available setting, e_setting_property property, bool state)
{
    if ((uint8_t)setting < (uint8_t)SETTING_NUM_SETTINGS)
    {
        if (state == true)
            setting_list[setting].state |=  property;
        else
            setting_list[setting].state &= ~property;
    }
    else
    {
        /* not valid */
    }
}

static uint16_t setting_compute_crc(e_settings_available setting)
{
    uint16_t computed_crc;

    computed_crc = PERSISTENT_CRC_SEED;
    computed_crc = crc16_1021(computed_crc, setting_list[setting].u_setting_value.byte_array[0U]);
    computed_crc = crc16_1021(computed_crc, setting_list[setting].u_setting_value.byte_array[1U]);
    computed_crc = crc16_1021(computed_crc, setting_list[setting].u_setting_value.byte_array[2U]);
    computed_crc = crc16_1021(computed_crc, setting_list[setting].u_setting_value.byte_array[3U]);

    return computed_crc;
}

static void setting_save_to_storage(e_settings_available setting)
{

    uint16_t crc;
    uint8_t  byte = 0U;
    uint16_t addr = PERSISTENT_DATA_START + ((uint16_t)setting * 6U);
    bool to_write = false;

    to_write |= setting_has_property(setting, SETTING_STATE_CHANGED);
    to_write |= (setting_has_property(setting, SETTING_STATE_VALID) == false) ? true : false;

    if (to_write == true)
    {

        /* changed, save it and compute the CRC value */
        persistent_write(addr++, setting_list[setting].u_setting_value.byte_array[byte++]);
        persistent_write(addr++, setting_list[setting].u_setting_value.byte_array[byte++]);
        persistent_write(addr++, setting_list[setting].u_setting_value.byte_array[byte++]);
        persistent_write(addr++, setting_list[setting].u_setting_value.byte_array[byte++]);

        /* Compute the CRC */
        crc = setting_compute_crc(setting);

        /* write the CRC */
        persistent_write(addr++, crc & 0xFF);
        persistent_write(addr++, (crc >> 8) & 0xFF);
    }
    else
    {
        /* not changed, no need to save */
    }

}
#include "stdio.h"
void settings_read_from_storage(void)
{

    uint16_t stored_crc;
    uint16_t computed_crc;
    uint16_t param;
    uint16_t addr = PERSISTENT_DATA_START;

    /* Read ALL */
    for (param = 0U; param < (uint16_t)SETTING_NUM_SETTINGS; param++)
    {

        /* Read data first */
        setting_list[param].u_setting_value.byte_array[0U] = persistent_read(addr++);
        setting_list[param].u_setting_value.byte_array[1U] = persistent_read(addr++);
        setting_list[param].u_setting_value.byte_array[2U] = persistent_read(addr++);
        setting_list[param].u_setting_value.byte_array[3U] = persistent_read(addr++);

        /* Read CRC finally */
        stored_crc = persistent_read(addr++);
        stored_crc |= (uint16_t)(((uint16_t)persistent_read(addr++)) << 8U);

        /* Compute the CRC */
        computed_crc = setting_compute_crc((e_settings_available)param);

        if (computed_crc != stored_crc)
        {
            /* invalid crc ! */
            setting_set_property((e_settings_available)param, SETTING_STATE_VALID, false);
        }
        else
        {
            /* valid crc */
            setting_set_property((e_settings_available)param, SETTING_STATE_VALID, true);
        }

    }

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
        setting_set_property(setting, SETTING_STATE_CHANGED, true);
    }
}
void setting_set_2(e_settings_available setting, uint16_t value)
{
    bool valid;
    valid = setting_valid_type(setting, SETTING_TYPE_VAL2);

    if (valid == true)
    {
        setting_list[setting].u_setting_value.byte2 = value;
        setting_set_property(setting, SETTING_STATE_CHANGED, true);
    }
}
void setting_set_4(e_settings_available setting, uint32_t value)
{
    bool valid;
    valid = setting_valid_type(setting, SETTING_TYPE_VAL4);

    if (valid == true)
    {
        setting_list[setting].u_setting_value.byte4 = value;
        setting_set_property(setting, SETTING_STATE_CHANGED, true);
    }
}
void setting_set_bool(e_settings_available setting, bool value)
{
    bool valid;
    valid = setting_valid_type(setting, SETTING_TYPE_BOOL);

    if (valid == true)
    {
        setting_list[setting].u_setting_value.byteB = value;
        setting_set_property(setting, SETTING_STATE_CHANGED, true);
    }
}
