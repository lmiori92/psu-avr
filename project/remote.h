/*
 * remote.h
 *
 *  Created on: 09 mag 2016
 *      Author: lorenzo
 */

#ifndef REMOTE_H_
#define REMOTE_H_

#include <stdint.h>

#define MAGIC_START         0x68656c6cUL//0xC0FFEE00UL
#define MAGIC_END           0xDEADBEEFUL

typedef struct
{
    uint32_t magic_start;
    uint8_t  node_id;
    uint8_t  len;
    uint16_t crc;
    uint32_t magic_end;
} t_remote_datagram;

#endif /* REMOTE_H_ */
