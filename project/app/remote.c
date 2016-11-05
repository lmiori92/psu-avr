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
#include "inc/uart.h"    /* UART primitives */
#include "inc/system.h"  /* Critical section handlers */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* External Library */
#include "lorenzlib/lib.h"     /* CRC16 CCITT, FIFO */

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

static t_fifo   rmt_rcv_fifo;
static t_fifo   rmt_snd_fifo;

static const char  DGRAM_MAGIC_START[] = { '\x00', '\xEE', '\xFF', '\xC0' };
static const char  DGRAM_MAGIC_END  [] = { '\xEF', '\xBE', '\xAD', '\xDE' };

/** Precalculate header "data" size for the needed fields */
#define DATAGRAM_HEADER_MAGIC_START_SIZE    (sizeof(DGRAM_MAGIC_START))
#define DATAGRAM_HEADER_MAGIC_END_SIZE      (sizeof(DGRAM_MAGIC_END))
#define DATAGRAM_HEADER_SIZE                (sizeof(t_remote_datagram_hdr))
#define DATAGRAM_HEADER_RAW_SIZE            (DATAGRAM_HEADER_SIZE + \
                                                (DATAGRAM_HEADER_MAGIC_START_SIZE * \
                                                        DATAGRAM_HEADER_MAGIC_END_SIZE))

void remote_init(void)
{
    /* use the FIFO routines without a buffer */
    fifo_init(&rmt_rcv_fifo, NULL, DGRAM_RCV_BUFFER_LEN);
    fifo_init(&rmt_snd_fifo, NULL, DGRAM_SND_BUFFER_LEN);
}

void remote_periodic(bool tick)
{
    if ((tick == true) && (remote_rcv_sm.tmo_cnt < DGRAM_RCV_TIMEOUT_TICKS))
    {
        /* increment the counter */
        remote_rcv_sm.tmo_cnt++;
    }
    else
    {
        /* do not overflow the counter */
    }

}

e_error remote_datagram_to_buffer(t_remote_datagram_hdr *datagram, uint8_t *buffer, uint8_t size)
{
    if (size >= DATAGRAM_HEADER_RAW_SIZE)
    {
        /* enough large buffer */

        /* add the initial magic */
        *(buffer++) = DGRAM_MAGIC_START[0];
        *(buffer++) = DGRAM_MAGIC_START[1];
        *(buffer++) = DGRAM_MAGIC_START[2];
        *(buffer++) = DGRAM_MAGIC_START[3];

        /* length and CRC */
        *(buffer++) = datagram->len;
        *(buffer++) = (uint8_t)datagram->crc;
        *(buffer++) = (uint8_t)(datagram->crc >> 8U);

        /* add the final magic */
        *(buffer++) = DGRAM_MAGIC_END[0];
        *(buffer++) = DGRAM_MAGIC_END[1];
        *(buffer++) = DGRAM_MAGIC_END[2];
        *(buffer++) = DGRAM_MAGIC_END[3];

        return E_OK;
    }
    else
    {
        /* refuse to overflow ! */
        return E_OVERFLOW;
    }
}

static void remote_receive_buffer_alloc(t_remote_datagram_buffer** datagram_buf)
{
    /* get the buffer at buffer's head */
    *datagram_buf = &(remote_rcv_buf[rmt_rcv_fifo.head]);
    /* push operation */
    fifo_push(&rmt_rcv_fifo, '\0');
}

static void remote_receive_buffer_pop(t_remote_datagram_buffer** datagram_buf)
{
    /* get the buffer at buffer's tail */
    *datagram_buf = &(remote_rcv_buf[rmt_rcv_fifo.tail]);
    /* pop operation */
    fifo_pop(&rmt_rcv_fifo, NULL);
}

void remote_send_alloc(t_remote_datagram_buffer** datagram_buf)
{
    /* get the buffer at buffer's head */
    *datagram_buf = &(remote_snd_buf[rmt_snd_fifo.head]);
}

void remote_send_push(void)
{
    /* perform the CRC generation */
    /* Calc the CRC */
    (void)remote_calc_crc_buffer_and_compare(remote_snd_buf[rmt_snd_fifo.head].data,
                                             remote_snd_buf[rmt_snd_fifo.head].datagram.len,
                                             0U,
                                             &remote_snd_buf[rmt_snd_fifo.head].datagram.crc);
    /* push operation */
    fifo_push(&rmt_snd_fifo, '\0');
}

static void remote_send_buffer_pop(t_remote_datagram_buffer** datagram_buf)
{
    /* get the buffer at buffer's tail */
    *datagram_buf = &(remote_snd_buf[rmt_snd_fifo.tail]);
    /* pop operation */
    fifo_pop(&rmt_snd_fifo, NULL);
}

bool remote_receive_get(t_remote_datagram_buffer *datagram)
{
    t_remote_datagram_buffer *buf = NULL;
    remote_receive_buffer_pop(&buf);

    if (buf != NULL && datagram != NULL)
    {
        /* get a copy */
        (void)memcpy(datagram, buf, sizeof(t_remote_datagram_buffer));

        return true;
    }
    else
    {
        return false;
    }
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
void remote_buffer_to_datagram(uint8_t input)
{

    /* Run the state machine for each received byte */
    if (remote_rcv_sm.tmo_cnt > DGRAM_RCV_TIMEOUT_TICKS)
    {
        /* timed out - restart the state machine */
        remote_rcv_sm.state = DGRAM_RCV_MAGIC_START;
        remote_rcv_sm.state_prev = DGRAM_RCV_MAGIC_END;   /* force state change to reuse code */
    }
    else
    {
        /* not timed-out */
    }

    /* reset timeout */
    remote_rcv_sm.tmo_cnt = 0;

    if (remote_rcv_sm.state != remote_rcv_sm.state_prev)
    {
        /* reset timeouts and indexes at state change */
        remote_rcv_sm.temp = 0;
        remote_rcv_sm.buf_index = 0;
        remote_rcv_sm.state_prev = remote_rcv_sm.state;
    }

    switch(remote_rcv_sm.state)
    {
        case DGRAM_RCV_MAGIC_START:
            /* Stay in there until the magic sequence is received */

            if ((char)input == DGRAM_MAGIC_START[remote_rcv_sm.temp])
            {
                remote_rcv_sm.temp++;
            }
            else
            {
                remote_rcv_sm.temp = 0;
            }

            if (remote_rcv_sm.temp >= sizeof(DGRAM_MAGIC_START))
            {
                /* reset counter */
                remote_rcv_sm.temp = 0;
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
                }
            }
            else
            {
                /* Invalid magic or out of sync */
                /* Stay listening for the magic sequence */
            }

            break;
        case DGRAM_RCV_HEADER:

            /* Use pointer arithmetic to optimize the call:
             * Pick the start address, sum it up with the buf index
             * and finally set the value at the location to the input */
            *((uint8_t*)&((remote_rcv_sm.datagram_buf->datagram)) + remote_rcv_sm.buf_index) = input;
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

            if ((char)input == DGRAM_MAGIC_END[remote_rcv_sm.temp])
            {
                remote_rcv_sm.temp++;
            }
            else
            {
                remote_rcv_sm.temp = 0;
            }

            if (remote_rcv_sm.temp >= sizeof(DGRAM_MAGIC_END))
            {
                /* reset counter */
                remote_rcv_sm.temp = 0;
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

}

void datagram_buffer_to_remote(void)
{

    /* Just a non interrupt driven stub to start with */

    t_remote_datagram_buffer *buf = NULL;

    uint8_t i = 0;
    uint8_t datagram_metadata[DATAGRAM_HEADER_RAW_SIZE];

    remote_send_buffer_pop(&buf);

    if (buf != NULL)
    {

        remote_datagram_to_buffer(&buf->datagram, datagram_metadata, DATAGRAM_HEADER_RAW_SIZE);

        for (i = 0; i < DATAGRAM_HEADER_RAW_SIZE; i++)
        {
            uart_putchar(datagram_metadata[i], NULL);
        }
        for (i = 0; i < buf->datagram.len; i++)
        {
            uart_putchar(buf->data[i], NULL);
        }
    }
    else
    {
        /* nothing to send */
    }

}

