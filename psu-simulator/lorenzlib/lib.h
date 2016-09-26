/*
 * lib.h
 *
 *  Created on: 20 mag 2016
 *      Author: lorenzo
 */

#ifndef LIB_H_
#define LIB_H_

#include <stdint.h>
#include <stdbool.h>

typedef uint16_t t_value_type;

typedef struct
{
    t_value_type raw;
    t_value_type scaled;
} t_value;

typedef struct _t_value_scale
{
    t_value_type min;
    t_value_type max;
    t_value_type min_scaled;
    t_value_type max_scaled;
} t_value_scale;

typedef struct
{
    uint16_t output;
    uint16_t alpha;

    uint32_t output_last;
} t_low_pass_filter;

typedef struct
{
    uint8_t *buf;
    uint8_t head;
    uint8_t tail;
    uint8_t size;
} t_fifo;

/* Definitions */
void lib_uint32_to_bytes(uint32_t input, uint8_t *lo, uint8_t *milo, uint8_t *hilo, uint8_t *hi);
uint16_t lib_bytes_to_uint16(uint8_t lo, uint8_t hi);
void lib_uint16_to_bytes(uint16_t input, uint8_t *lo, uint8_t *hi);
void lib_sum(uint16_t *value, uint16_t limit, uint16_t diff);
void lib_diff(uint16_t *value, uint16_t diff);
void lib_limit(t_value *value, t_value_scale *scale);
void lib_scale(t_value *value, t_value_scale *scale);

uint16_t crc16_1021(uint16_t old_crc, uint8_t data);

void low_pass_filter(uint16_t input, t_low_pass_filter *filter);

void fifo_init(t_fifo * f, uint8_t* buf, uint8_t size);
bool fifo_pop(t_fifo *f, uint8_t *byte);
bool fifo_push(t_fifo *f, uint8_t byte);

#endif /* LIB_H_ */
