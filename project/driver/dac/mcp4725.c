/*
 * mcp4725.c
 *
 *  Created on: 13 set 2016
 *      Author: lorenzo
 */

#include "inc/i2c_master.h"

#include "mcp4725.h"

static uint8_t buffer[4U];

void mcp_dac_init(void)
{
    mcp_dac_write_eeprom(0U);
    /* write a zero value */
    mcp_dac_write(0U);
}

void mcp_dac_write(uint16_t value)
{
    /* Prepare the buffer */
    /* we always perform writes (R/W bit is 0) */
    buffer[0U] = MCP_DAC_ADDRESS;
    buffer[1U] = (uint8_t)(((uint16_t)(value >> 8U)) & 0x0FU);
    buffer[2U] = (uint8_t)((uint16_t)(value));
    /* Update the DAC register using the Fast Mode */
    i2c_transfer_set_data(buffer, 3U);
    i2c_transfer_start();
    i2c_transfer_successful();
}

void mcp_dac_write_eeprom(uint16_t value)
{
    /* Prepare the buffer */
    buffer[1U] = (1 << MCP_DAC_C1_BIT) | (1 << MCP_DAC_C0_BIT);
    buffer[2U] = (uint8_t)((uint16_t)(value >> 8U));
    buffer[3U] = (uint8_t)((uint16_t)(value & 0x0FU) << 4U);
    /* Update the DAC register using the EEPROM+DAC Write Mode */
    i2c_transfer_set_data(buffer, 4U);
}

uint16_t mcp_dac_get_resolution(void)
{
    /* 12 effective bits */
    return 4095U;
}
