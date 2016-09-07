/*
 * eeprom.c
 *
 *  Created on: 30 lug 2016
 *      Author: lorenzo
 */

#include "eeprom.h"

#include "avr/eeprom.h"

void persistent_init(uint16_t *size)
{
    /* just in case: bootloader is/was performing some stuff yet to be done */
    eeprom_busy_wait();

    *size = E2END;
}

uint8_t persistent_read(uint16_t address)
{
    return eeprom_read_byte((uint8_t*)&address);
}

void    persistent_write(uint16_t address, uint8_t byte)
{
    eeprom_write_byte((uint8_t*)&address, byte);
}

void    persistent_deinit(void)
{
    /* placeholder */
}
