/*
 * mcp4725.h
 *
 *  Created on: 13 set 2016
 *      Author: lorenzo
 */

#ifndef HAL_DRIVER_ADC_MCP4725_H_
#define HAL_DRIVER_ADC_MCP4725_H_

#include "stdint.h"

/** Device address pin 0 */
#define MCP_DAC_A0              (1U)
/** Device address pin 1 */
#define MCP_DAC_A1              (1U)
/** Device address pin 2 */
#define MCP_DAC_A2              (0U)

/** Device i2c address */
#define MCP_DAC_ADDRESS         ((0xC0U) | (MCP_DAC_A2 << 3U) | (MCP_DAC_A1 << 2U)| (MCP_DAC_A0 << 1U))

/** Device C0/1/2 bits location */
#define MCP_DAC_Cx_BYTE         (0U)
/** Device C0 bit position */
#define MCP_DAC_C0_BIT          (5U)
/** Device C1 bit position */
#define MCP_DAC_C1_BIT          (6U)
/** Device C2 bit position */
#define MCP_DAC_C2_BIT          (7U)

/** Device PD0/1 bits location */
#define MCP_DAC_PDx_BYTE         (0U)
/** Device PDx bits shift on alternative write */
#define MCP_DAC_PDx_SHIFT_ALT_W  (2U)
/** Device PD0 bit position */
#define MCP_DAC_PD0_BIT          (4U)
/** Device PD1 bit position */
#define MCP_DAC_PD1_BIT          (5U)

/** Device general call RESET */
#define MCP_DAC_GEN_CALL_RST     (0x06U)
/** Device general call WAKE-UP */
#define MCP_DAC_GEN_CALL_WKUP    (0x09U)

void mcp_dac_init(void);
void mcp_dac_write(uint16_t value);
void mcp_dac_write_eeprom(uint16_t value);
uint16_t mcp_dac_get_resolution(void);

#endif /* HAL_DRIVER_ADC_MCP4725_H_ */
