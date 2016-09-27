/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Lorenzo Miori (C) 2016 [ 3M4|L: memoryS60<at>gmail.com ]

    Version History
        * 1.0 initial

*/

#ifndef __I2C_I2C_MASTER__
#define __I2C_I2C_MASTER__

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"

/* TWI Miscellaneous status codes */
#define TWI_NO_STATE               0xF8  /**< No relevant state information available; TWINT = 0 */
#define TWI_TIMEOUT                0x01  /**< Bus error due to a timeout */
#define TWI_BUS_ERROR              0x00  /**< Bus error due to an illegal START or STOP condition */

void    i2c_init(void);
uint8_t i2c_busy(void);
uint8_t i2c_get_state_info(void);

void    i2c_transfer_start(void);
void    i2c_transfer_set_data(uint8_t *data, uint8_t len);
uint8_t i2c_transfer_successful(void);

#endif      /* __I2C_I2C_MASTER__ */
