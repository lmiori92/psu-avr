/*
 * eeprom.c
 *
 *  Created on: 30 lug 2016
 *      Author: lorenzo
 */

#include "eeprom.h"

#include <stdio.h>

FILE* eeprom_image = NULL;

void persistent_init(uint16_t *size)
{
    /* open the fake eeprom image */
    eeprom_image = fopen("eeprom.bin", "r+");

    *size = 1024U;
}

uint8_t persistent_read(uint16_t address)
{
    uint8_t byte;

    if (eeprom_image != NULL)
    {
        fseek(eeprom_image, address, SEEK_SET);
        fread(&byte, 1U, 1U, eeprom_image);
    }
    else
    {
        /* not available, return unprogrammed byte */
        byte = 0xFFU;
    }

    return byte;
}

void    persistent_write(uint16_t address, uint8_t byte)
{
    if (eeprom_image != NULL)
    {
        fseek(eeprom_image, address, SEEK_SET);
        fwrite(&byte, 1U, 1U, eeprom_image);
    }
    else
    {
        /* not available */
    }
}

void    persistent_deinit(void)
{
    /* placeholder */
}
