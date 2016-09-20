/*
 * ads1015.h
 *
 *  Created on: 14 set 2016
 *      Author: lorenzo
 */

#ifndef HAL_DRIVER_ADC_ADS1015_H_
#define HAL_DRIVER_ADC_ADS1015_H_

#include "stdint.h"

/* ADS1015 Address values */

/**  */
#define ADS_ADC_A0                  (0U)
/**  */
#define ADS_ADC_A1                  (0U)
/**  */
#define ADS_ADC_ADDRESS_W           ((0x90U) | (ADS_ADC_A1 << 2) | (ADS_ADC_A0 << 1))
/**  */
#define ADS_ADC_ADDRESS_R           (ADS_ADC_ADDRESS_W | 1U)

/* ADS1015 Register Pointer values */

/**  */
#define ADS_ADC_CONV_REG            (0U)
/**  */
#define ADS_ADC_CONF_REG            (1U)
/**  */
#define ADS_ADC_LO_THRESH_REG       (2U)
/**  */
#define ADS_ADC_HI_THRESH_REG       (3U)

/* ADS1015 Config Register bit positions */

/**  */
#define ADS_ADC_COMP_QUE            (0U)
/**  */
#define ADS_ADC_COMP_LAT            (2U)
/**  */
#define ADS_ADC_COMP_POL            (3U)
/**  */
#define ADS_ADC_COMP_MODE           (4U)
/**  */
#define ADS_ADC_DR                  (5U)
/**  */
#define ADS_ADC_MODE                (8U)
/**  */
#define ADS_ADC_PGA                 (9U)
/**  */
#define ADS_ADC_MUX                 (12U)
/**  */
#define ADS_ADC_OS                  (15U)

/* Programmable Gain Amplifier values */

/**  */
#define ADS_ADC_PGA_6_144           (0U)
/**  */
#define ADS_ADC_PGA_4_096           (1U)
/**  */
#define ADS_ADC_PGA_2_048           (2U)
/**  */
#define ADS_ADC_PGA_1_024           (3U)
/**  */
#define ADS_ADC_PGA_0_512           (4U)
/**  */
#define ADS_ADC_PGA_0_256           (5U)

/* Samples Per Second value */

/**  */
#define ADS_ADC_DR_128_SPS          (0U)
/**  */
#define ADS_ADC_DR_250_SPS          (1U)
/**  */
#define ADS_ADC_DR_490_SPS          (2U)
/**  */
#define ADS_ADC_DR_920_SPS          (3U)
/**  */
#define ADS_ADC_DR_1600_SPS         (4U)
/**  */
#define ADS_ADC_DR_2400_SPS         (5U)
/**  */
#define ADS_ADC_DR_3300_SPS         (6U)

/**  */
#define ADS_ADC_MUX_AIN0_AIN1       (0U)
/**  */
#define ADS_ADC_MUX_AIN0_AIN3       (1U)
/**  */
#define ADS_ADC_MUX_AIN1_AIN3       (2U)
/**  */
#define ADS_ADC_MUX_AIN2_AIN3       (3U)
/**  */
#define ADS_ADC_MUX_AIN0_GND        (4U)
/**  */
#define ADS_ADC_MUX_AIN1_GND        (5U)
/**  */
#define ADS_ADC_MUX_AIN2_GND        (6U)
/**  */
#define ADS_ADC_MUX_AIN3_GND        (7U)

void ads_init(void);
void ads_select_register(uint8_t reg);
uint16_t ads_read(void);
void ads_write_config(uint16_t config);
uint16_t ads_get_config(void);

#endif /* HAL_DRIVER_ADC_ADS1015_H_ */
