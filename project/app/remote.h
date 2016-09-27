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

/**
 * @file remote.h
 * @author Lorenzo Miori
 * @date May 2016
 * @brief The remote transmission protocol.
 */

#ifndef REMOTE_H_
#define REMOTE_H_

#include "error.h"

#include <stdbool.h>
#include <stdint.h>

#define DGRAM_MASTER_NODE_ID          0U

#define DGRAM_MAGIC_START_RX          0xC0FFEE00UL
#define DGRAM_MAGIC_END_RX            0xDEADBEEFUL
#define DGRAM_MAGIC_START             0x00EEFFC0UL
#define DGRAM_MAGIC_END               0xEFBEADDEUL

#define DGRAM_RCV_DATA_MAX      20U
#define DGRAM_RCV_TIMEOUT_US    1000000U
#define DGRAM_RCV_BUFFER_LEN    5U          /**< Adjust me if out of RAM :-) */
#define DGRAM_SND_BUFFER_LEN    5U          /**< Adjust me if out of RAM :-) */

typedef enum
{
    DGRAM_RCV_MAGIC_START,  /**< Waiting for the magic sync */
    DGRAM_RCV_HEADER,       /**< Receiving header data (not CRC protected) */
    DGRAM_RCV_MAGIC_END,    /**< Waiting for the magic sync */
    DGRAM_RCV_DATA,         /**< Receiving data (CRC protected) */
} e_datagram_receive_state;

typedef enum
{
    DATATYPE_NULL,       /**< Datagram Header Only (data len = 0) */
    DATATYPE_CONFIG,     /**< Datagram Data carries config data */
    DATATYPE_READOUTS,   /**< Datagram Data carries readouts data */
    DATATYPE_SETPOINTS,  /**< Datagram Data carries setpoints data */
    DATAYPE_DEBUG        /**< Datagram Data carries debug data */
} e_datatype;

#define member_size(type, member) sizeof(((type *)0)->member)

typedef struct
{
    uint32_t magic_start;
    uint8_t  node_id;
    uint8_t  len;
    uint16_t crc;
    uint32_t magic_end;
} t_remote_datagram;

/** Precalculate header "data" size for the needed fields */
#define DATAGRAM_HEADER_SIZE    (member_size(t_remote_datagram, node_id) + member_size(t_remote_datagram, len) + member_size(t_remote_datagram, crc))
#define DATAGRAM_HEADER_START(datagram)   (datagram.node_id)
#define DATAGRAM_HEADER_PTR_START(datagram)   (datagram->node_id)

typedef struct
{
    t_remote_datagram datagram;
    uint8_t data[DGRAM_RCV_DATA_MAX];
    uint32_t timestamp;
} t_remote_datagram_buffer;

typedef struct
{
    e_datagram_receive_state   state;
    e_datagram_receive_state   state_prev;
    uint32_t                   timeout;
    uint32_t                   temp;
    uint8_t                    buf_index;
    t_remote_datagram_buffer   *datagram_buf;
} t_remote_receive_state_machine;

/** Buffer management **/
void remote_buffer_get_oldest(t_remote_datagram_buffer *static_buffer, t_remote_datagram_buffer** datagram_buf);
void remote_receive_buffer_get_oldest(t_remote_datagram_buffer** datagram_buf);
void remote_send_buffer_get_oldest(t_remote_datagram_buffer** datagram_buf);

void remote_buffer_alloc(t_remote_datagram_buffer *static_buffer, t_remote_datagram_buffer** datagram_buf);
void remote_receive_buffer_alloc(t_remote_datagram_buffer** datagram_buf);
void remote_send_buffer_alloc(t_remote_datagram_buffer** datagram_buf);

bool remote_receive_buffer_get(t_remote_datagram_buffer *datagram);
void remote_send_buffer_send(uint32_t timestamp, t_remote_datagram_buffer *rem_buf);

/** utilities **/
bool remote_calc_crc_buffer_and_compare(uint8_t *buffer, uint8_t len, uint16_t expected_crc, uint16_t *calc_crc);
e_error remote_datagram_to_buffer(t_remote_datagram *datagram, uint8_t *buffer, uint8_t size);

/** mid-level transmit and receive state machines **/
void remote_buffer_to_datagram(uint32_t timestamp, uint8_t input);
void datagram_buffer_to_remote(void);

#endif /* REMOTE_H_ */
