/*
 * eeprom.h
 *
 *  Created on: 30 lug 2016
 *      Author: lorenzo
 */

#ifndef EEPROM_H_
#define EEPROM_H_

#include "stdint.h"

void    persistent_init(uint16_t *size);

uint8_t persistent_read(uint16_t address);
void    persistent_write(uint16_t address, uint8_t byte);

void    persistent_deinit(void);

#endif /* EEPROM_H_ */
