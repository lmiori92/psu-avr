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

/* Definitions */
static void lib_uint16_to_bytes(uint16_t input, uint8_t *lo, uint8_t *hi);
static uint16_t lib_bytes_to_uint16(uint8_t lo, uint8_t hi);

/* Remote application level buffers */
static t_remote_datagram_buffer remote_dgram_rcv_copy;

void uart_received(uint8_t byte)
{

    /* NOTE: interrupt callback. Pay attention to execution time... */

    /* Call the state machine with a single byte... */
    remote_buffer_to_datagram(byte);

}

static void remote_encode_setpoint(t_remote_datagram_buffer *datagram, t_psu_channel *channel)
{

    datagram->datagram.node_id     = channel->remote_node;
    datagram->datagram.magic_start = DGRAM_MAGIC_START;
    datagram->datagram.magic_end   = DGRAM_MAGIC_END;

    /* "Serialize" data (not really, it only works on the same architecture) */

    /* Datatype */
    datagram->data[0] = DATATYPE_SETPOINTS;
    /* Voltage setpoint for the remote channel */
    lib_uint16_to_bytes(channel->voltage_setpoint.value.raw, &datagram->data[1], &datagram->data[2]);
    /* Current setpoint for the remote channel */
    lib_uint16_to_bytes(channel->current_setpoint.value.raw, &datagram->data[3], &datagram->data[4]);

    /* set byte length */
    datagram->datagram.len         = 5U;

    /* Calc the CRC */
    (void)remote_calc_crc_buffer_and_compare(datagram->data, datagram->datagram.len, 0U, &datagram->datagram.crc);

}

static void remote_encode_readout(t_remote_datagram_buffer *datagram, t_psu_channel *channel)
{

    datagram->datagram.node_id     = channel->remote_node;
    datagram->datagram.magic_start = DGRAM_MAGIC_START;
    datagram->datagram.magic_end   = DGRAM_MAGIC_END;

    /* "Serialize" data (not really, it only works on the same architecture) */

    /* Datatype */
    datagram->data[0] = DATATYPE_READOUTS;
    /* Voltage measurement for the remote channel */
    lib_uint16_to_bytes(channel->voltage_readout.value.raw, &datagram->data[1], &datagram->data[2]);
    /* Current measurement for the remote channel */
    lib_uint16_to_bytes(channel->current_readout.value.raw, &datagram->data[3], &datagram->data[4]);

    /* set byte length */
    datagram->datagram.len         = 5U;

    /* Calc the CRC */
    (void)remote_calc_crc_buffer_and_compare(datagram->data, datagram->datagram.len, 0U, &datagram->datagram.crc);

}

static void remote_decode_readout(t_remote_datagram_buffer *datagram, t_psu_channel *channel)
{
    if ((datagram != NULL) && (channel != NULL) && (datagram->datagram.len >= 5U))
    {
        /* Voltage setpoint for the remote channel */
        channel->voltage_readout.value.raw = lib_bytes_to_uint16(datagram->data[1], datagram->data[2]);
        /* Current setpoint for the remote channel */
        channel->current_readout.value.raw = lib_bytes_to_uint16(datagram->data[3], datagram->data[4]);
    }
}

static void remote_encode_config(t_remote_datagram_buffer *datagram, t_psu_channel *channel)
{
    if ((datagram != NULL) && (channel != NULL))
    {
        /* STUB */
    }
}

static void remote_decode_config(t_remote_datagram_buffer *datagram, t_psu_channel *channel)
{
    if ((datagram != NULL) && (channel != NULL))
    {
        /* STUB */
        /* Config received, set to operational */
        channel->state = PSU_STATE_OPERATIONAL;
    }
}

t_psu_channel* psu_get_channel_from_node_id(uint8_t node_id)
{
    uint16_t i = 0;

    t_psu_channel* ret = NULL;

    for (i = 0; i < PSU_CHANNEL_NUM; i++)
    {
        if (psu_channels[i].remote_node == node_id)
        {
            ret = &psu_channels[i];
        }
    }

    return ret;
}

/**
 * Parse the received datagram(s)
 */
static void remote_decode_datagram(void)
{
    bool new;
    bool crc_ok;
    uint8_t i = 0;

    do
    {
        new = remote_receive_buffer_get(&remote_dgram_rcv_copy);
        if (new == true)
        {
            crc_ok = remote_calc_crc_buffer_and_compare(remote_dgram_rcv_copy.data, remote_dgram_rcv_copy.datagram.len, remote_dgram_rcv_copy.datagram.crc, NULL);
            if ((crc_ok == true) && (remote_dgram_rcv_copy.datagram.len > 0U))
            {

                switch (remote_dgram_rcv_copy.data[0])
                {
                case DATAYPE_DEBUG:
                    /* debug data (usually a string) */
                    uart_putstring(remote_dgram_rcv_copy.data);
                    uart_putstring("\r\n");
                    break;
                case DATATYPE_CONFIG:
                    /* config data */
                    remote_decode_config(&remote_dgram_rcv_copy, psu_get_channel_from_node_id(remote_dgram_rcv_copy.datagram.node_id));
                    break;
                case DATATYPE_READOUTS:
                    /* readout data */
                    remote_decode_readout(&remote_dgram_rcv_copy, psu_get_channel_from_node_id(remote_dgram_rcv_copy.datagram.node_id));
                    break;
                case DATATYPE_SETPOINTS:
                    /* setpoints data */
                    break;
                default:
                    break;
                }
            }
        }
        i++;
    }
    while ((new == true) && (i < DGRAM_RCV_BUFFER_LEN));
}

static void remote_encode_datagram(e_datatype type, t_psu_channel *channel)
{

    t_remote_datagram_buffer *rem_buf;
    remote_send_buffer_alloc(&rem_buf);

    if (rem_buf != NULL)
    {

        switch (type)
        {
        case DATAYPE_DEBUG:
            /* debug data (usually a string) */
            break;
        case DATATYPE_CONFIG:
            /* config data */
            remote_encode_config(rem_buf, channel);
            break;
        case DATATYPE_READOUTS:
            /* readout data */
            remote_encode_readout(rem_buf, channel);
            break;
        case DATATYPE_SETPOINTS:
            /* setpoints data */
            break;
        default:
            break;
        }

        /* set the datagram as sendable */
        remote_send_buffer_send(rem_buf);
    }
    else
    {
        /* Send buffer overflow */
        uart_putstring("send ovf\r\n");
    }
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
    //stdout = &uart_output;
    //stdin  = &uart_input;

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

static void input_processing(void)
{

    uint8_t i;

    /* Parse remote datagrams */
    remote_decode_datagram();

    for (i = 0; i < (uint8_t)PSU_CHANNEL_NUM; i++)
    {
        if (psu_channels[i].remote_node == 0U)
        {
            /* Local channel */
            adc_processing(&psu_channels[i]);
        }
        else
        {
            /* Slave(s)<->Master communication takes care of that */
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
            /* Slave(s)<->Master communication takes care of that */
            remote_encode_datagram(DATATYPE_SETPOINTS, &psu_channels[i]);
        }
    }

}
/*
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
*/

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
    uart_putstring("\x1B[2J\x1B[H");

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
//        remote_datagram_to_buffer(&datagram, datagram_metadata, sizeof(t_remote_datagram));

//        for (i = 0; i < sizeof(t_remote_datagram); i++)
        {
//            uart_putchar(datagram_metadata[i], NULL);
        }
//        for (i = 0; i < datagram.len; i++)
        {
//            uart_putchar(buffer[i], NULL);
        }

    }

}
