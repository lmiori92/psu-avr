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

#include "error.h"
#include "remote.h"
#include "time.h"    /* g_s_timestamp (maybe try to generalize and design without this contraint) */
#include "uart.h"    /* UART primitives */
#include "lib.h"     /* CRC16 CCITT */
#include "system.h"  /* Critical section handlers */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define REMOTE_ENTER_CRITICAL_SECTION       system_interrupt_disable();   /**< Call it when handling a shared variable */
#define REMOTE_EXIT_CRITICAL_SECTION        system_interrupt_enable();    /**< Call it when done handling a shared variable */

static t_remote_receive_state_machine remote_rcv_sm =
{
    DGRAM_RCV_MAGIC_START,
    DGRAM_RCV_MAGIC_START,
    0U,
    0U,
    0U,
    NULL
};

static t_remote_datagram_buffer remote_rcv_buf[DGRAM_RCV_BUFFER_LEN];
static t_remote_datagram_buffer remote_snd_buf[DGRAM_SND_BUFFER_LEN];

e_error remote_datagram_to_buffer(t_remote_datagram *datagram, uint8_t *buffer, uint8_t size)
{
    if (size >= sizeof(t_remote_datagram))
    {
        /* enough large buffer */
        (void)memcpy(buffer, datagram, sizeof(t_remote_datagram));
        /*
        lib_uint32_to_bytes(datagram->magic_start, buffer, buffer + 1, buffer + 2, buffer + 3);
        buffer+=4;
        *buffer = datagram->node_id;
        buffer++;
        *buffer = datagram->len;
        buffer++;
        lib_uint16_to_bytes(datagram->crc, buffer, buffer + 1);
        buffer+=2;
        lib_uint32_to_bytes(datagram->magic_end, buffer, buffer + 1, buffer + 2, buffer + 3);
        buffer+=4;
        */
        return E_OK;
    }
    else
    {
        /* refuse to overflow ! */
        return E_OVERFLOW;
    }
}

void remote_buffer_alloc(t_remote_datagram_buffer *static_buffer, t_remote_datagram_buffer** datagram_buf)
{
    uint8_t i;
    *datagram_buf = NULL;
    for (i = 0; (i < DGRAM_RCV_BUFFER_LEN) || (i == 0xFFU); i++)  /* ...do not overflow if 0xFF! */
    {
        if (static_buffer[i].timestamp == 0U)
        {
            *datagram_buf = &(static_buffer[i]);
        }
    }
}

void remote_receive_buffer_alloc(t_remote_datagram_buffer** datagram_buf)
{
    /* wrapper */
    remote_buffer_alloc(remote_rcv_buf, datagram_buf);
}

void remote_send_buffer_alloc(t_remote_datagram_buffer** datagram_buf)
{
    /* wrapper */
    remote_buffer_alloc(remote_snd_buf, datagram_buf);
}

void remote_buffer_get_oldest(t_remote_datagram_buffer *static_buffer, t_remote_datagram_buffer** datagram_buf)
{
    uint8_t i;
    uint8_t id = 0xFF;
    uint32_t oldest = 0xFFFFFFFFU;
    *datagram_buf = NULL;
    for (i = 0; (i < DGRAM_RCV_BUFFER_LEN) || (i == 0xFFU); i++)  /* ...do not overflow if 0xFF! */
    {
        if ((static_buffer[i].timestamp > 0U) && (static_buffer[i].timestamp < oldest))
        {
            id = i;
            oldest = static_buffer[i].timestamp;
        }
    }

    if (id != 0xFF) *datagram_buf = &(static_buffer[id]);
}

void remote_receive_buffer_get_oldest(t_remote_datagram_buffer** datagram_buf)
{
    remote_buffer_get_oldest(remote_rcv_buf, datagram_buf);
}

void remote_send_buffer_get_oldest(t_remote_datagram_buffer** datagram_buf)
{
    remote_buffer_get_oldest(remote_snd_buf, datagram_buf);
}

bool remote_receive_buffer_get(t_remote_datagram_buffer *datagram)
{
    t_remote_datagram_buffer *buf = NULL;
    remote_receive_buffer_get_oldest(&buf);

    if (buf != NULL && datagram != NULL)
    {
        /* get a copy */
        (void)memcpy(datagram, buf, sizeof(t_remote_datagram_buffer));
        /* immediately reset timestamp
         * CAUTION: critical section! */
        REMOTE_ENTER_CRITICAL_SECTION;
        buf->timestamp = 0U;
        REMOTE_EXIT_CRITICAL_SECTION;
        return true;
    }
    else
    {
        return false;
    }
}

void remote_send_buffer_send(t_remote_datagram_buffer *rem_buf)
{
    REMOTE_ENTER_CRITICAL_SECTION;

    /* mark as sendable */
    rem_buf->timestamp = g_timestamp;

    REMOTE_EXIT_CRITICAL_SECTION;
}

bool remote_calc_crc_buffer_and_compare(uint8_t *buffer, uint8_t len, uint16_t expected_crc, uint16_t *calc_crc)
{
    uint16_t i;
    uint16_t crc = 0;

    for (i = 0; i < len; i++)
    {
        crc = crc16_1021(crc, buffer[i]);
    }

    if (calc_crc != NULL) *calc_crc = crc;

    if ((crc != expected_crc) || (len == 0U))
    {
        /* CRC is unexpected or length is zero -> something failed */
        return false;
    }
    else
    {
        return true;
    }
}

/**
 * The execution time of this state machine shall be
 * as short as possible and moreover it shall be well
 * within the 100us timer time window to avoid accuracy
 * problems
 */
e_error remote_buffer_to_datagram(uint8_t input)
{

    e_error err = E_OK;    /* save the error code or so */

    /* Run the state machine for each received byte */
    if (g_timestamp > remote_rcv_sm.timeout)
    {
        /* timed out - restart the state machine */
        remote_rcv_sm.state = DGRAM_RCV_MAGIC_START;
        remote_rcv_sm.state_prev = DGRAM_RCV_MAGIC_END;   /* force state change to reuse code */
        err = E_TIMEOUT;
    }

    if (remote_rcv_sm.state != remote_rcv_sm.state_prev)
    {
        /* reset timeouts and indexes at state change */
        remote_rcv_sm.temp = 0;
        remote_rcv_sm.buf_index = 0;
        remote_rcv_sm.timeout = g_timestamp + DGRAM_RCV_TIMEOUT_US;
        remote_rcv_sm.state_prev = remote_rcv_sm.state;
    }

    switch(remote_rcv_sm.state)
    {
        case DGRAM_RCV_MAGIC_START:
            /* Stay in there until the magic sequence is received */

            remote_rcv_sm.temp <<= 8U;    /* shift 1 byte to the left (to make space for the new one) */
            remote_rcv_sm.temp |= input;

            if (remote_rcv_sm.temp == DGRAM_MAGIC_START_RX)
            {
                /* new datagram is incoming: allocate a buffer slot, if possible */
                remote_receive_buffer_alloc(&remote_rcv_sm.datagram_buf);
                if (remote_rcv_sm.datagram_buf != NULL)
                {
                    /* clear some fields */
                    remote_rcv_sm.datagram_buf->datagram.crc = 0;
                    remote_rcv_sm.datagram_buf->datagram.len = 0;
                    /* alright, datagram header synchronized! */
                    remote_rcv_sm.state = DGRAM_RCV_HEADER;
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
            remote_rcv_sm.timeout = g_timestamp + DGRAM_RCV_TIMEOUT_US;

            break;
        case DGRAM_RCV_HEADER:

            /* Use pointer arithmetic to optimize the ISR call:
             * Pick the start address, sum it up with the buf index
             * and finally set the value at the location to the input */
            *((uint8_t*)&(DATAGRAM_HEADER_START(remote_rcv_sm.datagram_buf->datagram)) + remote_rcv_sm.buf_index) = input;
            remote_rcv_sm.buf_index++;

            if (remote_rcv_sm.buf_index >= DATAGRAM_HEADER_SIZE)
            {
                /* header completely received */
                remote_rcv_sm.state = DGRAM_RCV_MAGIC_END;
                remote_rcv_sm.buf_index = 0;
            }

            break;
        case DGRAM_RCV_MAGIC_END:
            /* Stay in there until the magic sequence is received */

            remote_rcv_sm.temp <<= 8U;    /* shift 1 byte to the left (to make space for the new one) */
            remote_rcv_sm.temp |= input;

            err = E_OK;

            if (remote_rcv_sm.temp == DGRAM_MAGIC_END_RX)
            {
                /* alright, datagram header synchronized! */
                remote_rcv_sm.state = DGRAM_RCV_DATA;
            }
            else
            {
                /* Invalid magic or out of sync, restart */
            }

            break;
        case DGRAM_RCV_DATA:

            remote_rcv_sm.datagram_buf->data[remote_rcv_sm.buf_index] = input;
            remote_rcv_sm.buf_index++;
            if ((remote_rcv_sm.buf_index >= remote_rcv_sm.datagram_buf->datagram.len) || (remote_rcv_sm.buf_index >= DGRAM_RCV_DATA_MAX))
            {
                /* data transfer completed */
                remote_rcv_sm.state = DGRAM_RCV_MAGIC_START;
                /* set timestamp to indicate buffer slot not free and to give an order */
                remote_rcv_sm.datagram_buf->timestamp = g_timestamp;
/*                printf("*%d %d\n", buf_index, datagram_buf->datagram.len);    */
                remote_rcv_sm.buf_index = 0;
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
    return err;

}

void datagram_buffer_to_remote(void)
{

    /* Just a non interrupt driven stub to start with */

    t_remote_datagram_buffer *buf = NULL;

    uint8_t i = 0;
    uint8_t datagram_metadata[sizeof(t_remote_datagram)];

    remote_send_buffer_get_oldest(&buf);

    if (buf != NULL)
    {

        remote_datagram_to_buffer(&buf->datagram, datagram_metadata, sizeof(t_remote_datagram));

        for (i = 0; i < sizeof(t_remote_datagram); i++)
        {
            uart_putchar(datagram_metadata[i], NULL);
        }
        for (i = 0; i < buf->datagram.len; i++)
        {
            uart_putchar(buf->data[i], NULL);
        }

        /* immediately reset timestamp
         * CAUTION: critical section! */
        REMOTE_ENTER_CRITICAL_SECTION;
        buf->timestamp = 0U;
        REMOTE_EXIT_CRITICAL_SECTION;
    }
    else
    {
        /* nothing to send */
    }

}

