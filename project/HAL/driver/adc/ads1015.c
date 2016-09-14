/*
 * ads1015.c
 *
 *  Created on: 14 set 2016
 *      Author: lorenzo
 */

#include "ads1015.h"

#include "../../../i2c/i2c_master.h"

static uint8_t buf[5U];
static uint16_t config;

void ads_write_config(void)
{

    /* Set the address byte */
    buf[0] = ADS_ADC_ADDRESS_W;
    /* Point to the config register */
    buf[1] = ADS_ADC_CONF_REG;
    /* load the config register */
    buf[2] = (uint8_t)((uint16_t)(config >> 8U));
    buf[3] = (uint8_t)config;

    i2c_transfer_set_data(buf, 5U);
    i2c_transfer_start();
    i2c_transfer_successful();

}

void ads_init(void)
{

    config = ADS_ADC_DR_3300_SPS << ADS_ADC_DR;

    ads_write_config();

}
