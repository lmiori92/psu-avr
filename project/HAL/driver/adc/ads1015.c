/*
 * ads1015.c
 *
 *  Created on: 14 set 2016
 *      Author: lorenzo
 */

#include "ads1015.h"

#include "../../../i2c/i2c_master.h"

static uint8_t buf[5U];

void ads_write_config(uint16_t config)
{

    /* Set the address byte */
    buf[0] = ADS_ADC_ADDRESS_W;
    /* Point to the config register */
    buf[1] = ADS_ADC_CONF_REG;
    /* load the config register */
    buf[2] = (uint8_t)((uint16_t)(config >> 8U));
    buf[3] = (uint8_t)config;

    i2c_transfer_set_data(buf, 4U);
    i2c_transfer_start();
    i2c_transfer_successful();

}

void ads_select_register(uint8_t reg)
{
    /* Set the address byte */
    buf[0] = ADS_ADC_ADDRESS_W;
    /* Point to the config register */
    buf[1] = reg;
    i2c_transfer_set_data(buf, 2U);
    i2c_transfer_start();
    i2c_transfer_successful();
}

uint16_t ads_get_config(void)
{

    /* Point to the config register */
    ads_select_register(ADS_ADC_CONF_REG);

    /* Set the address byte */
    buf[0] = ADS_ADC_ADDRESS_R;

    i2c_transfer_set_data(buf, 3U);
    i2c_transfer_start();
    i2c_transfer_successful();

    return (uint16_t)((uint16_t)buf[1] << 8) | (uint16_t)buf[2];
}

uint16_t ads_read(void)
{

    /* Point to the config register */
    ads_select_register(ADS_ADC_CONV_REG);

    /* Set the address byte */
    buf[0] = ADS_ADC_ADDRESS_R;

    i2c_transfer_set_data(buf, 3U);
    i2c_transfer_start();
    i2c_transfer_successful();

    return (uint16_t)(((uint16_t)buf[1] << 8) | ((uint16_t)buf[2]));
}

void ads_init(void)
{

    uint16_t config/* = 0x4285U*/;

    /* Set the intial configuration */
    config  = (0U << ADS_ADC_COMP_QUE);
    config |= (0U << ADS_ADC_COMP_LAT);
    config |= (0U << ADS_ADC_COMP_POL);
    config |= (0U << ADS_ADC_COMP_MODE);
    config |= (ADS_ADC_DR_1600_SPS << ADS_ADC_DR);
    config |= (0U << ADS_ADC_MODE);
    config |= (ADS_ADC_PGA_0_512 << ADS_ADC_PGA);
    config |= (ADS_ADC_MUX_AIN0_GND << ADS_ADC_MUX);
    config |= (0U << ADS_ADC_OS);

    ads_write_config(config);

}
