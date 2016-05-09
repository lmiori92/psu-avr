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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/delay.h>
#include <avr/crc16.h>
#include "adc.h"
#include "encoder.h"
#include "pwm.h"
#include "psu.h"
#include "time.h"
#include "uart.h"
#include "remote.h"
#include "system.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define ADC_RESOLUTION      1023U

/* GLOBALS */
static t_psu_channel psu_channels[PSU_CHANNEL_NUM];

typedef enum _e_error
{
    E_OK,
    E_UNKNOWN,
    E_CRC,
    E_MAGIC_START,
    E_MAGIC_END,
    E_OVERFLOW,
    E_TIMEOUT,
    E_RECEIVING_HDR /**< Datagram header not yet complete */
} e_error;

/* Definitions */
static void lib_uint16_to_bytes(uint16_t input, uint8_t *lo, uint8_t *hi);
static uint16_t lib_bytes_to_uint16(uint8_t lo, uint8_t hi);

#define BUF_SIZE    10
static uint8_t buffer[BUF_SIZE];
static t_remote_datagram datagram_received;

static e_error remote_datagram_to_buffer(t_remote_datagram *datagram, uint8_t *buffer, uint8_t size)
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

typedef enum
{
    DGRAM_RCV_MAGIC_START,
    DGRAM_RCV_HEADER,
    DGRAM_RCV_MAGIC_END,

} e_datagram_receive_state;

#define DGRAM_RCV_TIMEOUT_US        1000000U

static e_error remote_buffer_to_datagram(t_remote_datagram *datagram, uint8_t *buffer, uint8_t size)
{
    static e_datagram_receive_state state = DGRAM_RCV_MAGIC_START;
    static e_datagram_receive_state state_prev = DGRAM_RCV_MAGIC_START;
    static uint32_t timeout = 0;
    static uint8_t buf[4] = { 0 };
    static uint8_t buf_index = 0;
    uint8_t i = 0;
    e_error err = E_UNKNOWN;

    for (i = 0; i < size; i++)
    {

        /* Run the state machine for each received byte */

        switch(state)
        {
        case DGRAM_RCV_MAGIC_START:
            /* Stay in there until the magic sequence is received */

            buf[buf_index] = buffer[i];
            buf_index++;

            err = E_RECEIVING_HDR;

            if (buf_index >= 4)
            {
                if (    (buf[0] == (uint8_t)MAGIC_START) &&
                        (buf[1] == (uint8_t)((uint32_t)MAGIC_START >> 8)) &&
                        (buf[2] == (uint8_t)((uint32_t)MAGIC_START >> 16)) &&
                        (buf[3] == (uint8_t)((uint32_t)MAGIC_START >> 24))
                     )
                {
                    /* alright, datagram header synchronized! */
                    state = DGRAM_RCV_HEADER;
                }
                else
                {
                    /* Invalid magic or out of sync */
                    err = E_MAGIC_START;
                    buf_index = 0;
                }
            }

            break;
        default:
            /* Fatal error */
            break;
        }

        if ((buf_index > 0) && (g_timestamp > timeout))
        {
            /* timed out - restart the state machine */
            //state = DGRAM_RCV_MAGIC_START;
            //err = E_TIMEOUT;
            //timeout = g_timestamp + DGRAM_RCV_TIMEOUT_US;
            //buf_index = 0;
        }

        if (state != state_prev)
        {
            buf_index = 0;
            timeout = g_timestamp + DGRAM_RCV_TIMEOUT_US;
            state_prev = state;
        }

    }

    return err;

}

void uart_received(uint8_t byte)
{
    /* Call the state machine with a single byte... */
    remote_buffer_to_datagram(&datagram_received, &byte, 1U);
}

static bool remote_calc_crc_buffer_and_compare(uint8_t *buffer, uint8_t len, uint16_t expected_crc, uint16_t *calc_crc)
{
    uint16_t crc;
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

static e_error remote_master_recv(t_remote_datagram *datagram, uint8_t *buffer, t_psu_channel *channel)
{

    bool crc_ok;
    e_error err = E_UNKNOWN;

    crc_ok = remote_calc_crc_buffer_and_compare(buffer, BUF_SIZE, datagram->crc, NULL);

    if (crc_ok == false)
    {
        /* invalid data */
        err = E_CRC;
    }
    else
    {
        /* data is valid */

        /* Voltage setpoint for the remote channel */
        channel->voltage_readout.value.raw = lib_bytes_to_uint16(buffer[0], buffer[1]);
        /* Current setpoint for the remote channel */
        channel->current_readout.value.raw = lib_bytes_to_uint16(buffer[2], buffer[3]);
    }

    return err;
}

static e_error remote_master_send(t_remote_datagram *datagram, uint8_t *buffer, t_psu_channel *channel)
{

    datagram->len = BUF_SIZE;
    datagram->crc = 0;
    datagram->node_id = channel->remote_node;
    datagram->magic_start = MAGIC_START;
    datagram->magic_end = MAGIC_END;

    /* "Serialize" data (not really, it only works on the same architecture) */

    /* Voltage setpoint for the remote channel */
    lib_uint16_to_bytes(channel->voltage_setpoint.value.raw, &buffer[0], &buffer[1]);
    /* Current setpoint for the remote channel */
    lib_uint16_to_bytes(channel->current_setpoint.value.raw, &buffer[2], &buffer[3]);

    /* Calc the CRC */
    (void)remote_calc_crc_buffer_and_compare(buffer, BUF_SIZE, 0U, &datagram->crc);

    return E_OK;
}

static void lib_uint32_to_bytes(uint32_t input, uint8_t *lo, uint8_t *milo, uint8_t *hilo, uint8_t *hi)
{
    *lo   = (uint8_t)input;
    *milo = (uint8_t)(input >> 8U);
    *hilo = (uint8_t)(input >> 16U);
    *hi   = (uint8_t)(input >> 16U);
}

static void lib_uint16_to_bytes(uint16_t input, uint8_t *lo, uint8_t *hi)
{
    *lo   = (uint8_t)input;
    *hi = (uint8_t)(input >> 8U);
}

static uint16_t lib_bytes_to_uint16(uint8_t lo, uint8_t hi)
{
    uint16_t output;
    output  = (uint8_t)lo;
    output |= (uint8_t)(hi << 8U);
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
    t_value_type temp;
    float range_ratio = ((float)scale->max_scaled - scale->min_scaled) / ((float)scale->max - scale->min);

    /* Remove the min value from the raw value */
    temp = value->raw - scale->min;
    /* Multiply the raw value by the ratio between the destination range and the origin range */
    temp = (float)temp * range_ratio;
    /* Offset the value by the min scaled value */
    temp += scale->min_scaled;
    /* Assign the value to the output */
    value->scaled = (t_value_type)temp;
}

static void init_channel(t_psu_channel *channel, e_psu_channel psu_ch)
{

    switch(psu_ch)
    {
        case PSU_CHANNEL_0:
            channel->remote_node         = 0U;
            channel->voltage_adc_channel = ADC_0;
            channel->current_adc_channel = ADC_1;
            channel->voltage_pwm_channel = PWM_CHANNEL_0;
            channel->current_pwm_channel = PWM_CHANNEL_1;
            break;
        case PSU_CHANNEL_1:
            channel->remote_node         = 1U;
            channel->voltage_adc_channel = ADC_2;
            channel->current_adc_channel = ADC_3;
            channel->voltage_pwm_channel = PWM_CHANNEL_2;
            channel->current_pwm_channel = PWM_CHANNEL_3;
            break;
        default:
            /* No channel selected */
            break;
    }

    channel->voltage_readout.scale.min = 0;
    channel->voltage_readout.scale.max = ADC_RESOLUTION;  /* ADC steps */
    channel->voltage_readout.scale.min_scaled = 0;
    channel->voltage_readout.scale.max_scaled = 28500;    /* Voltage */

    channel->current_readout.scale.min = 0;
    channel->current_readout.scale.max = ADC_RESOLUTION;  /* ADC steps */
    channel->current_readout.scale.min_scaled = 0;
    channel->current_readout.scale.max_scaled = 2048;     /* Voltage */

    channel->voltage_setpoint.scale.min = 0;
    channel->voltage_setpoint.scale.max = 28500;
    channel->voltage_setpoint.scale.min_scaled = 0;
    channel->voltage_setpoint.scale.max_scaled = pwm_get_resolution(channel->voltage_pwm_channel);

    channel->current_setpoint.scale.min = 0;
    channel->current_setpoint.scale.max = 28500;
    channel->current_setpoint.scale.min_scaled = 0;
    channel->current_setpoint.scale.max_scaled = pwm_get_resolution(channel->current_pwm_channel);

}

static void init_psu(t_psu_channel *channel)
{

    uint8_t i;

    for (i = 0; i < (uint8_t)PSU_CHANNEL_NUM; i++)
    {
        init_channel(&channel[i], (e_psu_channel)i);
    }

}

static const uint16_t smoothing_deltat[] =
{
    65051, 54845, 47930, 42693, 38477, 34948, 31913, 29251, 26880, 24742, 22796,
    21010, 19360, 17827, 16395, 15051, 13786, 12590, 11457, 10380, 9354,
    8374, 7437, 6538, 5675, 4845, 4046, 3275, 2530, 1810, 1113, 500
};

#define SMOOTHING_SIZE      (sizeof(smoothing_deltat) / sizeof(smoothing_deltat[0]))

static const uint16_t smoothing_result[SMOOTHING_SIZE] =
{
    1, 1, 1, 20, 55, 90, 125, 160, 195, 230, 265, 300, 335, 370, 405, 440, 475,
    510, 545, 580, 615, 650, 685, 720, 755, 790, 825, 860, 895, 930, 965, 1000
};

static void encoder_event_callback(e_enc_event event, uint32_t delta_t)
{
    uint8_t i;
    uint16_t diff;
    for (i = 0; i < SMOOTHING_SIZE; i++)
    {
        if (delta_t >= smoothing_deltat[i])
        {
            diff = smoothing_result[i];
            if (event == ENC_EVT_LEFT)
            {
                lib_diff(&psu_channels[PSU_CHANNEL_0].voltage_setpoint.value.raw, diff);
            }
            else if (event == ENC_EVT_RIGHT)
            {
                lib_sum(&psu_channels[PSU_CHANNEL_0].voltage_setpoint.value.raw, psu_channels[PSU_CHANNEL_0].voltage_setpoint.scale.max, diff);
            }
            break;
        }
    }
}

static void init_io(void)
{

    cli();

    /* UART */
    uart_init();
    uart_callback(uart_received);
    stdout = &uart_output;
    stdin  = &uart_input;

    /* ADC */
    adc_init();

    /* PWM */
    pwm_init();

    /* System timer */
    timer_init();

    /* Encoder */
    encoder_init();
    encoder_set_callback(ENC_HW_0, encoder_event_callback);

    sei();

}

static void adc_processing(t_psu_channel *channel)
{

    /* Voltage */
    channel->voltage_readout.value.raw = adc_get(channel->voltage_adc_channel);

    /* Current */
    channel->current_readout.value.raw = adc_get(channel->current_adc_channel);

}

static void pwm_processing(t_psu_channel *channel)
{

    /* Voltage */
    pwm_set_duty(channel->voltage_pwm_channel, channel->voltage_setpoint.value.scaled);

    /* Current */
    pwm_set_duty(channel->current_pwm_channel, channel->current_setpoint.value.scaled);

}

static void psu_postprocessing(t_psu_channel *channel)
{
    /* Voltage Scaling */
    lib_scale(&channel->voltage_readout.value, &channel->voltage_readout.scale);
    /* Current Scaling */
    lib_scale(&channel->current_readout.value, &channel->current_readout.scale);
}

static void psu_preprocessing(t_psu_channel *channel)
{
    /* Voltage Scaling */
    lib_scale(&channel->voltage_setpoint.value, &channel->voltage_setpoint.scale);
    /* Current Scaling */
    lib_scale(&channel->current_setpoint.value, &channel->current_setpoint.scale);
}

/* testing */
static t_remote_datagram datagram;

static void input_processing(void)
{

    uint8_t i;

    for (i = 0; i < (uint8_t)PSU_CHANNEL_NUM; i++)
    {
        if (psu_channels[i].remote_node == 0U)
        {
            /* Local channel */
            adc_processing(&psu_channels[i]);
        }
        else
        {
            /* Slave(s)<->Master communication */
            remote_master_recv(&datagram, buffer, &psu_channels[i]);
        }

        /* Post-processing (scaling) of the values */
        psu_postprocessing(&psu_channels[i]);

    }

}

static void output_processing(void)
{
    uint8_t i;

    for (i = 0; i < (uint8_t)PSU_CHANNEL_NUM; i++)
    {
        /* Pre-processing (scaling) of the values */
        psu_preprocessing(&psu_channels[i]);

        if (psu_channels[i].remote_node == 0U)
        {
            /* Local channel */
            pwm_processing(&psu_channels[i]);
        }
        else
        {
            /* Slave(s)<->Master communication */
            // TODO NOTE THE 0, just for testing :-)
            remote_master_send(&datagram, buffer, &psu_channels[0]);
        }
    }

}

static void dbg_print_values(t_psu_channel *psu_chs, uint8_t num_psu)
{
    uint8_t i = 0;

    for (i = 0; i < num_psu; i++)
    {
        printf("PSU CH-%d\r\n", i);
        printf("%u mV", psu_chs[i].voltage_setpoint.value.raw);
        printf(" (%u)\r\n", psu_chs[i].voltage_setpoint.value.scaled);
        printf("%u mV", psu_chs[i].current_setpoint.value.raw);
        printf(" (%u)\r\n", psu_chs[i].current_setpoint.value.scaled);
    }
}



int main(void)
{

    /* System init */
    system_init();

    /* Initialize I/Os */
    init_io();

    /* Init ranges and precisions */
    init_psu(psu_channels);

/*
    channels[0].voltage_readout.value.raw = 250;
    lib_scale(&channels[0].voltage_readout.value, &channels[0].voltage_readout.scale);
    printf("Raw is %d and scaled is %d\r\n", channels[0].voltage_readout.value.raw, channels[0].voltage_readout.value.scaled);

    channels[0].voltage_readout.value.raw = 500;
    lib_scale(&channels[0].voltage_readout.value, &channels[0].voltage_readout.scale);
    printf("Raw is %d and scaled is %d\r\n", channels[0].voltage_readout.value.raw, channels[0].voltage_readout.value.scaled);

    channels[0].voltage_readout.value.raw = 750;
    lib_scale(&channels[0].voltage_readout.value, &channels[0].voltage_readout.scale);
    printf("Raw is %d and scaled is %d\r\n", channels[0].voltage_readout.value.raw, channels[0].voltage_readout.value.scaled);
*/
    printf("\x1B[2J\x1B[H");

    while (1)
    {
        /* Periodic functions */
        input_processing();

        /*
        psu_channels[0].voltage_setpoint.value.raw = 19856;
        observed an offset error of about 50mV
        note that the prototype breadboard does not have a separatly filtered and regulated 5V supply
        psu_channels[0].current_setpoint.value.raw = 1500;// = 2047; // observed an offset error of about 40mV
*/
        // to-do / to analyze: 1) absolute offset calibration
        //                     2) non linear behaviour correction (do measurements)

        /* Encoder periodic logic */
        //uint32_t error = (uint32_t)psu_channels[0].voltage_setpoint.value.raw;
        //if (error > psu_channels[0].voltage_readout.value.scaled) error = psu_channels[0].voltage_setpoint.value.raw - psu_channels[0].voltage_readout.value.scaled;
        //else error = psu_channels[0].voltage_readout.value.scaled - psu_channels[0].voltage_setpoint.value.raw;
//        printf("\x1B[2J\x1B[H");

//        printf("\033[0;0H");
//        dbg_print_values(psu_channels, PSU_CHANNEL_NUM);
        //remote_master_send(&datagram, buffer, &psu_channels[0]);


        /*
        printf("\x1B[2J\x1B[H");
        printf("%u mV", (uint32_t)psu_channels[0].voltage_setpoint.value.raw);
        printf("(%u mV)", (uint32_t)psu_channels[0].voltage_readout.value.scaled);
        printf("(error is %u mV)\r\n", (uint32_t)error);
        */
#ifdef TIMER_DEBUG
        /* Debug the timer */
        timer_debug();
#endif
        /* Output processing */
        output_processing();

        uint8_t i = 0;
        uint8_t datagram_metadata[sizeof(t_remote_datagram)];
        remote_datagram_to_buffer(&datagram, datagram_metadata, sizeof(t_remote_datagram));

        for (i = 0; i < sizeof(t_remote_datagram); i++)
        {
//            uart_putchar(datagram_metadata[i], NULL);
        }
        for (i = 0; i < datagram.len; i++)
        {
//            uart_putchar(buffer[i], NULL);
        }

    }

}
