/*
 * remote.c
 *
 *  Created on: 11 mag 2016
 *      Author: lorenzo
 */

#include "error.h"
#include "remote.h"
#include "time.h"   /* g_s_timestamp (maybe try to generalize and design without this contraint) */

#include <util/crc16.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static t_remote_datagram_buffer remote_rcv_buf[DGRAM_RCV_BUFFER_LEN];

e_error remote_datagram_to_buffer(t_remote_datagram *datagram, uint8_t *buffer, uint8_t size)
{
    if (size >= sizeof(t_remote_datagram))
    {
        /* enough large buffer */
        memcpy(&datagram[0], &buffer, sizeof(t_remote_datagram));
        return E_OK;
    }
    else
    {
        /* refuse to overflow ! */
        return E_OVERFLOW;
    }
}

void remote_receive_buffer_alloc(t_remote_datagram_buffer** datagram_buf)
{
    uint8_t i;
    *datagram_buf = NULL;
    for (i = 0; (i < DGRAM_RCV_BUFFER_LEN) || (i == 0xFFU); i++)  /* ...do not overflow if 0xFF! */
    {
        if (remote_rcv_buf[i].timestamp == 0U)
        {
            *datagram_buf = &(remote_rcv_buf[i]);
        }
    }
}

void remote_receive_buffer_get_oldest(t_remote_datagram_buffer** datagram_buf)
{
    uint8_t i;
    uint8_t id = 0xFF;
    uint32_t oldest = 0xFFFFFFFFU;
    *datagram_buf = NULL;
    for (i = 0; (i < DGRAM_RCV_BUFFER_LEN) || (i == 0xFFU); i++)  /* ...do not overflow if 0xFF! */
    {
        if ((remote_rcv_buf[i].timestamp > 0U) && (remote_rcv_buf[i].timestamp < oldest))
        {
            id = i;
            oldest = remote_rcv_buf[i].timestamp;
        }
    }

    if (id != 0xFF) *datagram_buf = &(remote_rcv_buf[id]);
}

bool remote_receive_buffer_get_oldest_copy(t_remote_datagram_buffer *datagram)
{
    t_remote_datagram_buffer *buf = NULL;
    remote_receive_buffer_get_oldest(&buf);

    if (buf != NULL && datagram != NULL)
    {
        /* get a copy */
        (void)memcpy(datagram, buf, sizeof(t_remote_datagram_buffer));
        /* immediately reset timestamp
         * CAUTION: critical section! */
        buf->timestamp = 0U;
        return true;
    }
    else
    {
        return false;
    }
}

e_error remote_buffer_to_datagram(uint8_t *input, uint8_t size)
{
    static e_datagram_receive_state state = DGRAM_RCV_MAGIC_START;
    static e_datagram_receive_state state_prev = DGRAM_RCV_MAGIC_START;
    static uint32_t timeout = 0;
    static uint8_t buf[DATAGRAM_HEADER_SIZE];
    static uint32_t temp;
    static uint8_t buf_index = 0;
    static t_remote_datagram_buffer *datagram_buf;
    uint8_t i = 0;
    e_error err = E_OK;

    for (i = 0; i < size; i++)
    {

        /* Run the state machine for each received byte */
        if (g_timestamp > timeout)
        {
            /* timed out - restart the state machine */
            state = DGRAM_RCV_MAGIC_START;
            state_prev = DGRAM_RCV_MAGIC_END;   /* force state change to reuse code */
            err = E_TIMEOUT;
        }

        if (state != state_prev)
        {
            temp = 0;
            buf_index = 0;
            timeout = g_timestamp + DGRAM_RCV_TIMEOUT_US;
            state_prev = state;
        }

        switch(state)
        {
        case DGRAM_RCV_MAGIC_START:
            /* Stay in there until the magic sequence is received */

            temp <<= 8U;    /* shift 1 byte to the left (to make space for the new one) */
            temp |= input[i];

            if (temp == DGRAM_MAGIC_START)
            {
                /* new datagram is incoming: allocate a buffer slot, if possible */
                remote_receive_buffer_alloc(&datagram_buf);
                if (datagram_buf != NULL)
                {
                    /* copy the magic number */
                    datagram_buf->datagram.magic_start = temp;
                    /* alright, datagram header synchronized! */
                    state = DGRAM_RCV_HEADER;
                }
                else
                {
                    /* Overflow! */
                    err = E_OVERFLOW;
                }
            }
            else
            {
                /* Invalid magic or out of sync */
                /* Stay listening for the magic sequence */
            }

            /* Never timeout in this state */
            timeout = g_timestamp + DGRAM_RCV_TIMEOUT_US;

            break;
        case DGRAM_RCV_HEADER:

            /* State transition happens, so this is safe */
            buf[buf_index] = input[i];
            buf_index++;

            if (buf_index >= DATAGRAM_HEADER_SIZE)
            {
                /* raw copy the received data. Cannot overflow due to
                 * the above macro which exactly calculates field size */
                (void)memcpy(&(DATAGRAM_HEADER_START(datagram_buf->datagram)), buf, DATAGRAM_HEADER_SIZE);
                /* header completely received */
                state = DGRAM_RCV_MAGIC_END;
                buf_index = 0;
            }

            break;
        case DGRAM_RCV_MAGIC_END:
            /* Stay in there until the magic sequence is received */

            temp <<= 8U;    /* shift 1 byte to the left (to make space for the new one) */
            temp |= input[i];

            err = E_OK;

            if (temp == DGRAM_MAGIC_END)
            {
                /* copy the magic number */
                datagram_buf->datagram.magic_end = temp;
                /* alright, datagram header synchronized! */
                state = DGRAM_RCV_DATA;
            }
            else
            {
                /* Invalid magic or out of sync, restart */
            }

            break;
        case DGRAM_RCV_DATA:

            datagram_buf->data[buf_index] = input[i];
            buf_index++;
            if ((buf_index >= datagram_buf->datagram.len) || (buf_index >= DGRAM_RCV_DATA_MAX))
            {
                /* data transfer completed */
                state = DGRAM_RCV_MAGIC_START;
                /* set timestamp to indicate buffer slot not free and to give an order */
                datagram_buf->timestamp = g_timestamp;
/*                printf("*%d %d\n", buf_index, datagram_buf->datagram.len);    */
                buf_index = 0;
            }
            else
            {
                /* transfer in progress */
            }

            break;
        default:
            /* Fatal error */
            break;
        }

    }

    return err;

}

bool remote_calc_crc_buffer_and_compare(uint8_t *buffer, uint8_t len, uint16_t expected_crc, uint16_t *calc_crc)
{
    uint16_t crc = 0;
    uint8_t i;

    for (i = 0; i < len; i++)
    {
        crc = _crc16_update(crc, buffer[i]);
    }

    if (calc_crc != NULL) *calc_crc = crc;

    if (crc != expected_crc)
    {
        return false;
    }
    else
    {
        return true;
    }
}





