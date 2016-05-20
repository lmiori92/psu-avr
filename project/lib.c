/*
 * lib.c
 *
 *  Created on: 20 mag 2016
 *      Author: lorenzo
 */

#include "lib.h"

void lib_uint32_to_bytes(uint32_t input, uint8_t *lo, uint8_t *milo, uint8_t *hilo, uint8_t *hi)
{
    *lo   = (uint8_t)input;
    *milo = (uint8_t)(input >> 8U);
    *hilo = (uint8_t)(input >> 16U);
    *hi   = (uint8_t)(input >> 24U);
}

void lib_uint16_to_bytes(uint16_t input, uint8_t *lo, uint8_t *hi)
{
    *lo   = (uint8_t)input;
    *hi = (uint8_t)(input >> 8U);
}

uint16_t lib_bytes_to_uint16(uint8_t lo, uint8_t hi)
{
    uint16_t output;
    output  = lo;
    output |= (uint16_t)(hi << 8U);
    return output;
}

void lib_sum(uint16_t *value, uint16_t limit, uint16_t diff)
{
    if (limit < *value)
    {
        *value = limit;
    }
    else
    {
        if ((limit - *value) > diff) *value  += diff;
        else                          *value  = limit;
    }
}

void lib_diff(uint16_t *value, uint16_t diff)
{
    if (*value > diff) *value -= diff;
    else               *value  = 0;
}

void lib_limit(t_value *value, t_value_scale *scale)
{
    /* bottom filter */
    if (value->raw < scale->min) value->scaled = scale->min;
    /* top filter */
    if (value->raw > scale->max) value->scaled = scale->max;
}

void lib_scale(t_value *value, t_value_scale *scale)
{
    uint32_t temp;

    /* Remove the min value from the raw value */
    temp = scale->max_scaled - scale->min_scaled;
    temp *= (value->raw - scale->min);
    /* Divide the raw value by the the destination range */
    temp /= (scale->max - scale->min);
    /* Assign the value to the output */
    value->scaled = temp;
    /* Offset the value by the min scaled value */
    value->scaled += scale->min_scaled;
}
