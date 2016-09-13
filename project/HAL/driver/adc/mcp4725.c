/*
 * mcp4725.c
 *
 *  Created on: 13 set 2016
 *      Author: lorenzo
 */

#include "../../../i2c/i2c_master.h"

#include "mcp4725.h"

static uint8_t buffer[4U];

void mcp_dac_init(void)
{
    /* write a zero value */
    mcp_dac_write(0U);
    /* we always perform writes (R/W bit is 0) */
    buffer[0U] = MCP_DAC_ADDRESS;
}

void mcp_dac_write(uint16_t value)
{
    /* Prepare the buffer */
    buffer[1U] = (uint8_t)((uint16_t)(value >> 12U) & 0xFU);
    buffer[2U] = (uint8_t)(value);
    /* Update the DAC register using the Fast Mode */
    i2c_transfer_set_data(buffer, 3U);
}

void mcp_dac_write_eeprom(uint16_t value)
{
    /* Prepare the buffer */
    buffer[1U] = (1 << MCP_DAC_C1_BIT) | (1 << MCP_DAC_C0_BIT);
    buffer[1U] = (uint8_t)((uint16_t)(value) & 0xFU);
    buffer[2U] = (uint8_t)(value << 4U);
    /* Update the DAC register using the Fast Mode */
    i2c_transfer_set_data(buffer, 4U);
}
