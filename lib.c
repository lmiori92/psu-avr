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

    Some parts by other authors whose copyright has been held
    by an explicit header on the function(s).

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

/*
 *
;----------------------------------------------------------------------
; PIC CRC16 (CRC-CCITT-16 0x1021)
;
;
;
;
; Background:
;
; Ashley Roll posted some code on the piclist (http://www.piclist.com)
; that implemented "CRC16" - or the CRC-CCITT-16 algorithm for the polynomial
; 0x1021. Tony K�bek and I took a few rounds optimizing the code and
; we ended up with a PIC implementation that was only 17 instructions
; (and 17 cycles, i.e. unlooped)!
;
; After further investigations, I found that the algorithm can be
; expressed:
;
;
; int x;
;
; x = ((crc>>8) ^ data) & 0xff;
; x ^= x>>4;
;
; crc = (crc << 8) ^ (x << 12) ^ (x <<5) ^ x;
;
; crc &= 0xffff;
;
; No claim is made here that this is the first time that this algorithm
; has been expressed this way. But, it's the first time I've seen like
; this. Using this as a guide, I wrote another routine in PIC assembly.
; Unfortunately, this one takes 17 cycles too.
;
; Digital Nemesis Pty Ltd
; www.digitalnemesis.com
; ash@digitalnemesis.com
;
; Original Code: Ashley Roll
; Optimisations: Scott Dattalo
;----------------------------------------------------------------------
 * */
uint16_t crc16_1021(uint16_t old_crc, uint8_t data)
{

    uint16_t crc;
    uint16_t x;

    x = ((old_crc >> 8) ^ data) & 0xff;
    x ^= x >> 4;

    crc = (old_crc << 8) ^ (x << 12) ^ (x << 5) ^ x;

    crc &= 0xffff;      /* enable this line for processors with more than 16 bits */

    return crc;

}

void low_pass_filter(uint16_t input, t_low_pass_filter *filter)
{
    uint32_t tmp;

    /* weigh the previous output */
    tmp = filter->alpha;
    tmp *= filter->output_last;
    /* scale the input value and add to it */
    tmp += 1000U * (uint32_t)input;
    tmp /= (filter->alpha + 1U);
    /* save last input value */
    filter->output_last = tmp;

    /* compute the scaled result */
    filter->output = (uint16_t)(filter->output_last / 1000U);
}
