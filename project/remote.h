/*
 * remote.h
 *
 *  Created on: 09 mag 2016
 *      Author: lorenzo
 */

#ifndef REMOTE_H_
#define REMOTE_H_

#include "error.h"

#include <stdbool.h>
#include <stdint.h>

#define DGRAM_MAGIC_START             0xC0FFEE00UL
#define DGRAM_MAGIC_END               0xDEADBEEFUL

#define DGRAM_RCV_DATA_MAX      16U
#define DGRAM_RCV_TIMEOUT_US    1000000U
#define DGRAM_RCV_BUFFER_LEN    5U

typedef enum
{
    DGRAM_RCV_MAGIC_START,
    DGRAM_RCV_HEADER,
    DGRAM_RCV_MAGIC_END,
    DGRAM_RCV_DATA,

} e_datagram_receive_state;

enum
{
    DATATYPE_NULL,      /**< Datagram Header Only (data len = 0) */
    DATATYPE_CONFIG,    /**< Datagram Data carries config data */
    DATATYPE_READOUTS,  /**< Datagram Data carries readouts data */
    DATAYPE_SETPOINTS,  /**< Datagram Data carries setpoints data */
    DATAYPE_DEBUG       /**< Datagram Data carries debug data */
};

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

e_error remote_datagram_to_buffer(t_remote_datagram *datagram, uint8_t *buffer, uint8_t size);
void remote_receive_buffer_alloc(t_remote_datagram_buffer** datagram_buf);
void remote_receive_buffer_get_oldest(t_remote_datagram_buffer** datagram_buf);
bool remote_receive_buffer_get_oldest_copy(t_remote_datagram_buffer *datagram);

e_error remote_buffer_to_datagram(uint8_t *input, uint8_t size);

bool remote_calc_crc_buffer_and_compare(uint8_t *buffer, uint8_t len, uint16_t expected_crc, uint16_t *calc_crc);

#endif /* REMOTE_H_ */
