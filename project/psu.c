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

#include "adc.h"
#include "display.h"
#include "display_config.h"
#include "encoder.h"
#include "lib.h"
#include "keypad.h"
#include "menu.h"
#include "pwm.h"
#include "psu.h"
#include "uart.h"
#include "remote.h"
#include "system.h"
#include "time_m.h"

/* Standard Library */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* DEFINES */

#define SMOOTHING_SIZE      (sizeof(smoothing_deltat) / sizeof(smoothing_deltat[0]))

/* CONSTANTS */
const uint8_t psu_channel_node_id_map[PSU_CHANNEL_NUM][2] =
{
        /* MASTER - SLAVE (0: local channel or disabled) */
        { 0, 1 },
        { 1, 0 }
};

static const uint16_t smoothing_deltat[] =
{
    65051, 54845, 47930, 42693, 38477, 34948, 31913, 29251, 26880, 24742, 22796,
    21010, 19360, 17827, 16395, 15051, 13786, 12590, 11457, 10380, 9354,
    8374, 7437, 6538, 5675, 4845, 4046, 3275, 2530, 1810, 1113, 500
};

static const uint16_t smoothing_result[SMOOTHING_SIZE] =
{
    1, 1, 1, 20, 55, 90, 125, 160, 195, 230, 265, 300, 335, 370, 405, 440, 475,
    510, 545, 580, 615, 650, 685, 720, 755, 790, 825, 860, 895, 930, 965, 1000
};

/* GLOBALS */
static t_psu_channel psu_channels[PSU_CHANNEL_NUM] =
{
        /* Channel 0 (local) */
        {
                .voltage_adc_channel = ADC_0,
                .current_adc_channel = ADC_1,
                .voltage_pwm_channel = PWM_CHANNEL_0,
                .current_pwm_channel = PWM_CHANNEL_1,
        },

        /* Channel 1 (remote) */
        {
                .voltage_adc_channel = ADC_2,
                .current_adc_channel = ADC_3,
                .voltage_pwm_channel = PWM_CHANNEL_2,
                .current_pwm_channel = PWM_CHANNEL_3,
        }
};

static t_application application;

/* Remote application level buffers */
static t_remote_datagram_buffer remote_dgram_rcv_copy;

/* MENU (testing) */
static e_psu_gui_menu menu_page = PSU_MENU_PSU;
static uint8_t asd = 234;
static t_menu_state menu_state = {0, 1, MENU_NOT_SELECTED};
static uint8_t bool_values[2] = { 1, 0 };
static char* bool_labels[2] = { "YES", "NO" };
static t_menu_extra_list menu_extra_3 = { 2U, 0U, bool_labels, bool_values };

static t_menu_item menu_item[4] = { {"A", (void*)&asd, MENU_TYPE_NUMERIC_8 },
                                    {"B", NULL, MENU_TYPE_NONE },
                                    {"C", (void*)&application.cycle_time, MENU_TYPE_NUMERIC_16 },
                                    {"PID", (void*)&menu_extra_3, MENU_TYPE_LIST }
                                    };

static void uart_received(uint8_t byte)
{

    /* NOTE: interrupt callback. Pay attention to execution time... */

    /* Call the state machine with a single byte... */
    remote_buffer_to_datagram(g_timestamp, byte);

}

static void remote_encode_setpoint(t_remote_datagram_buffer *datagram, t_psu_channel *channel)
{

    datagram->datagram.node_id     = channel->node_id;
    datagram->datagram.magic_start = DGRAM_MAGIC_START;
    datagram->datagram.magic_end   = DGRAM_MAGIC_END;

    /* Serialize data */

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

static void remote_decode_setpoint(t_remote_datagram_buffer *datagram, t_psu_channel *channel)
{
    if ((datagram != NULL) && (channel != NULL) && (datagram->datagram.len >= 5U))
    {
        /* Voltage setpoint for the remote channel */
        channel->voltage_setpoint.value.raw = lib_bytes_to_uint16(datagram->data[1], datagram->data[2]);
        /* Current setpoint for the remote channel */
        channel->current_setpoint.value.raw = lib_bytes_to_uint16(datagram->data[3], datagram->data[4]);
        /* heartbeat notify */
        channel->heartbeat_timestamp = g_timestamp;
    }
}

static void remote_encode_readout(t_remote_datagram_buffer *datagram, t_psu_channel *channel)
{

    datagram->datagram.node_id     = channel->node_id;
    datagram->datagram.magic_start = DGRAM_MAGIC_START;
    datagram->datagram.magic_end   = DGRAM_MAGIC_END;

    /* Serialize data */

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
        /* Voltage measurement for the remote channel */
        channel->voltage_readout.value.raw = lib_bytes_to_uint16(datagram->data[1], datagram->data[2]);
        /* Current measurement for the remote channel */
        channel->current_readout.value.raw = lib_bytes_to_uint16(datagram->data[3], datagram->data[4]);

        /* heartbeat notify */
        channel->heartbeat_timestamp = g_timestamp;
    }
}

static void remote_encode_config(t_remote_datagram_buffer *datagram, t_psu_channel *channel)
{
    if ((datagram != NULL) && (channel != NULL))
    {
        datagram->datagram.node_id     = channel->node_id;
        datagram->datagram.magic_start = DGRAM_MAGIC_START;
        datagram->datagram.magic_end   = DGRAM_MAGIC_END;

        /* Serialize data */

        /* Datatype */
        datagram->data[0] = DATATYPE_CONFIG;
        /* ADC voltage resolution */
        lib_uint16_to_bytes(channel->voltage_readout.scale.max, &datagram->data[1], &datagram->data[2]);
        /* ADC current resolution */
        lib_uint16_to_bytes(channel->current_readout.scale.max, &datagram->data[3], &datagram->data[4]);

        /* Voltage scaled maximum */
        lib_uint16_to_bytes(channel->voltage_readout.scale.max_scaled, &datagram->data[5], &datagram->data[6]);
        /* Current scaled maximum */
        lib_uint16_to_bytes(channel->current_readout.scale.max_scaled, &datagram->data[7], &datagram->data[8]);

        /* PWM voltage resolution */
        lib_uint16_to_bytes(channel->voltage_setpoint.scale.max, &datagram->data[9], &datagram->data[10]);
        /* PWM current resolution */
        lib_uint16_to_bytes(channel->current_setpoint.scale.max, &datagram->data[11], &datagram->data[12]);

        /* PWM Voltage scaled maximum */
        lib_uint16_to_bytes(channel->voltage_setpoint.scale.max_scaled, &datagram->data[13], &datagram->data[14]);
        /* PWM Current scaled maximum */
        lib_uint16_to_bytes(channel->current_setpoint.scale.max_scaled, &datagram->data[15], &datagram->data[16]);

        /* set byte length */
        datagram->datagram.len         = 17U;

        /* Calc the CRC */
        (void)remote_calc_crc_buffer_and_compare(datagram->data, datagram->datagram.len, 0U, &datagram->datagram.crc);
    }
}

static void remote_decode_config(t_remote_datagram_buffer *datagram, t_psu_channel *channel)
{
    if ((datagram != NULL) && (channel != NULL) && (datagram->datagram.len >= 17U))
    {

        channel->voltage_readout.scale.max = lib_bytes_to_uint16(datagram->data[1], datagram->data[2]);
        channel->current_readout.scale.max = lib_bytes_to_uint16(datagram->data[3], datagram->data[4]);
        channel->voltage_readout.scale.max_scaled = lib_bytes_to_uint16(datagram->data[5], datagram->data[6]);
        channel->current_readout.scale.max_scaled = lib_bytes_to_uint16(datagram->data[7], datagram->data[8]);
        channel->voltage_setpoint.scale.max = lib_bytes_to_uint16(datagram->data[9], datagram->data[10]);
        channel->current_setpoint.scale.max = lib_bytes_to_uint16(datagram->data[11], datagram->data[12]);
        channel->voltage_setpoint.scale.max_scaled = lib_bytes_to_uint16(datagram->data[13], datagram->data[14]);
        channel->current_setpoint.scale.max_scaled = lib_bytes_to_uint16(datagram->data[15], datagram->data[16]);

        /* Config received, set to operational */
        channel->state = PSU_STATE_OPERATIONAL;

        /* heartbeat notify */
        channel->heartbeat_timestamp = g_timestamp;

    }
}

static t_psu_channel* psu_get_channel_from_node_id(uint8_t node_id)
{
    uint16_t i = 0;

    t_psu_channel* ret = NULL;

    for (i = 0; i < PSU_CHANNEL_NUM; i++)
    {
        if (psu_channels[i].node_id == node_id)
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
    t_psu_channel *temp_ch;

    do
    {
        new = remote_receive_buffer_get(&remote_dgram_rcv_copy);
        if (new == true)
        {
            crc_ok = remote_calc_crc_buffer_and_compare(remote_dgram_rcv_copy.data, remote_dgram_rcv_copy.datagram.len, remote_dgram_rcv_copy.datagram.crc, NULL);
            if ((crc_ok == true) && (remote_dgram_rcv_copy.datagram.len > 0U))
            {

                /* Get the associated channel, if possible
                 * NOTE: channel id is divided by 2 (it is * 2 on slave!) */
                temp_ch = psu_get_channel_from_node_id(remote_dgram_rcv_copy.datagram.node_id);

                switch (remote_dgram_rcv_copy.data[0])
                {
                case DATAYPE_DEBUG:
                    /* debug data (usually a string) */
                    //uart_putstring(remote_dgram_rcv_copy.data);
                    //uart_putstring("\r\n");
                    break;
                case DATATYPE_CONFIG:
                    /* config data */
                    remote_decode_config(&remote_dgram_rcv_copy, temp_ch);

                    break;
                case DATATYPE_READOUTS:
                    /* readout data */
                    remote_decode_readout(&remote_dgram_rcv_copy, temp_ch);
                    break;
                case DATATYPE_SETPOINTS:
                    /* setpoints data */
                    remote_decode_setpoint(&remote_dgram_rcv_copy, temp_ch);
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
            remote_encode_setpoint(rem_buf, channel);
            break;
        default:
            break;
        }

        /* set the datagram as sendable */
        remote_send_buffer_send(g_timestamp, rem_buf);
    }
    else
    {
        /* Send buffer overflow */

    }
}

static void psu_select_channel(e_psu_channel channel, e_psu_setpoint setpoint)
{
    application.selected_setpoint = setpoint;
    application.selected_psu = channel;

    /* Cache the pointers for an optimized ISR routine */
    application.selected_psu_ptr = &psu_channels[channel];
    switch (setpoint)
    {
    case PSU_SETPOINT_CURRENT:
        application.selected_setpoint_ptr = &psu_channels[channel].current_setpoint;
        break;
    case PSU_SETPOINT_VOLTAGE:
        application.selected_setpoint_ptr = &psu_channels[channel].voltage_setpoint;
        break;
    }

}

static void psu_advance_selection(void)
{
    e_psu_channel ch = application.selected_psu;
    e_psu_setpoint sp = application.selected_setpoint;

    switch (application.selected_setpoint)
    {
    case PSU_SETPOINT_VOLTAGE:
        sp = PSU_SETPOINT_CURRENT;
        break;
    case PSU_SETPOINT_CURRENT:
        /* Switch to the other channel */
        switch (ch)
        {
        case PSU_CHANNEL_0:
            ch = PSU_CHANNEL_1;
            break;
        case PSU_CHANNEL_1:
            ch = PSU_CHANNEL_0;
            break;
        default:
            ch = PSU_CHANNEL_0;
            break;
        }

        sp = PSU_SETPOINT_VOLTAGE;

        break;
    }

    psu_select_channel(ch, sp);

}

static void psu_check_channel(t_psu_channel *psu_channel)
{

    switch(psu_channel->state)
    {
    case PSU_STATE_INIT:

        /* Send the channel's configuration */
        if ((psu_channel->master_or_slave == false) && (psu_channel->node_id > 0U))
        {
            /* Send 3 copies of config frames */
            remote_encode_datagram(DATATYPE_CONFIG, psu_channel);
        }
        else
        {
            /* No configuration to be done */
        }

        /* goto preoperational */
        psu_channel->state = PSU_STATE_PREOPERATIONAL;

        break;
    case PSU_STATE_PREOPERATIONAL:
        /* Preoperational */
        if (psu_channel->remote_or_local == false)
        {
            /* Immediatly do the transition to operational, if
             * local channel (i.e. hardware is ready) */
            psu_channel->state = PSU_STATE_OPERATIONAL;
        }
        else
        {
            /* wait for the config frame */
        }
        break;
    case PSU_STATE_OPERATIONAL:
        /* Check for timeouts (last timestamp is far in the past) */

        if ((g_timestamp - psu_channel->heartbeat_timestamp) > PSU_CHANNEL_TIMEOUT)
        {
            /* 100ms worth of data has been lost - or slave is locked-up */
            psu_channel->state = PSU_STATE_SAFE_STATE;
        }

        break;
    case PSU_STATE_SAFE_STATE:
        break;
    default:
        break;
    }
}

static void psu_init_channel(t_psu_channel *channel, e_psu_channel psu_ch, bool master_or_slave)
{

    /* Preoperational mode initially */
    channel->state             = PSU_STATE_INIT;

    /* Remote master or slave? */
    channel->master_or_slave = master_or_slave;

    /* Set nodeID */
    channel->node_id = psu_channel_node_id_map[psu_ch][master_or_slave ? 0U : 1U];

    /* Remote or local? */
    if (master_or_slave == true)
    {
        /* on a master only nodes with ID 0 are local */
        channel->remote_or_local = (channel->node_id > 0U) ? true : false;
    }
    else
    {
        /* on a slave, all channels are local */
        channel->remote_or_local = false;
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
    channel->current_setpoint.scale.max = 2048;
    channel->current_setpoint.scale.min_scaled = 0;
    channel->current_setpoint.scale.max_scaled = pwm_get_resolution(channel->current_pwm_channel);

    pid_Init(1,0,0, &channel->current_limit_pid);
}

static void psu_init(void)
{

    uint8_t i;

    for (i = 0; i < (uint8_t)PSU_CHANNEL_NUM; i++)
    {
        psu_init_channel(&psu_channels[i], (e_psu_channel)i, application.master_or_slave);
    }

    /* Start with a known selection */
    psu_select_channel(PSU_CHANNEL_0, PSU_SETPOINT_VOLTAGE);

}

static void encoder_event_callback(e_enc_event event, uint32_t delta_t)
{
    uint8_t i;
    uint16_t diff;

    if (event == ENC_EVT_CLICK_DOWN)
    {
        /* Click event */
        keypad_set_input(BUTTON_SELECT, false);
    }
    else if (event == ENC_EVT_CLICK_UP)
    {
        /* Release event */
        keypad_set_input(BUTTON_SELECT, true);
    }
    else
    {
        if (event == ENC_EVT_LEFT)
        {
            menu_event(MENU_EVENT_LEFT);
        }
        else if (event == ENC_EVT_RIGHT)
        {
            menu_event(MENU_EVENT_RIGHT);
        }
        /* Rotation event */
        for (i = 0; i < SMOOTHING_SIZE; i++)
        {
            if (delta_t >= smoothing_deltat[i])
            {
                diff = smoothing_result[i];
                if (event == ENC_EVT_LEFT)
                {
                    lib_diff(&application.selected_setpoint_ptr->value.raw, diff);
                }
                else if (event == ENC_EVT_RIGHT)
                {
                    lib_sum(&application.selected_setpoint_ptr->value.raw, application.selected_setpoint_ptr->scale.max, diff);
                }
                break;
            }
        }
    }
}

static void init_io(void)
{

    system_interrupt_disable();

    /* Read the coding pin to understand if master or slave */
    application.master_or_slave = system_coding_pin_read();

    /* UART */
    uart_init();
    uart_callback(uart_received);
    /* stdout = &uart_output; */
    /* stdin  = &uart_input; */

    /* ADC */
    adc_init();

    /* PWM */
    pwm_init();
    pwm_enable_channel(PWM_CHANNEL_0);
    pwm_enable_channel(PWM_CHANNEL_1);

    /* System timer */
    timer_init();

    /* Display: set the intended HAL and initialize the subsystem */
    display_select();
    display_init();

    /* Encoder */
    encoder_init();
    encoder_set_callback(ENC_HW_0, encoder_event_callback);

    /* Keypad */
    keypad_init();

    system_interrupt_enable();

}

static void psu_adc_processing(t_psu_channel *channel)
{
    /* Voltage */
    channel->voltage_readout.value.raw = adc_get(channel->voltage_adc_channel);
    /* Current */
    channel->current_readout.value.raw = adc_get(channel->current_adc_channel);
}

static void psu_pwm_processing(t_psu_channel *channel)
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

static void psu_input_processing(void)
{

    uint8_t i;

    /* Parse remote datagrams */
    remote_decode_datagram();

    for (i = 0; i < (uint8_t)PSU_CHANNEL_NUM; i++)
    {
        if (psu_channels[i].remote_or_local == false)
        {
            /* Local channel */
//            psu_adc_processing(&psu_channels[i]);
            /* handled in the high priority routine */

            /* Heartbeat */
            psu_channels[i].heartbeat_timestamp = g_timestamp;

            if ((psu_channels[i].master_or_slave == false) && (psu_channels[i].node_id > 0U))
            {
                remote_encode_datagram(DATATYPE_READOUTS, &psu_channels[i]);
            }

        }
        else
        {
            /* Slave(s)<->Master communication takes care of that */
        }

        /* Perform checks on the channel */
        psu_check_channel(&psu_channels[i]);

        /* Post-processing (scaling) of the values */
        psu_postprocessing(&psu_channels[i]);

    }

}

static void psu_output_processing(void)
{
    uint8_t i;

    for (i = 0; i < (uint8_t)PSU_CHANNEL_NUM; i++)
    {
        if (psu_channels[i].state == PSU_STATE_OPERATIONAL)
        {
            /* Pre-processing (scaling) of the values */
            psu_preprocessing(&psu_channels[i]);

            if (psu_channels[i].remote_or_local == false)
            {
                /* Local channel */

                /* handled in the high priority routine */

//                psu_pwm_processing(&psu_channels[i]);
            }
            else
            {
                if ((application.master_or_slave == true) && (psu_channels[i].node_id > 0U))
                {
                    /* Slave(s)<->Master communication takes care of that */
                    remote_encode_datagram(DATATYPE_SETPOINTS, &psu_channels[i]);
                }
                else
                {
                    /* Slave(s) are not sending setpoint values */
                }
            }
        }
    }

}

static uint8_t get_digit(uint16_t *val)
{
    *val = *val / 10U;
    return '0' + *val % 10U;
}

static void gui_print_measurement(e_psu_setpoint type, uint16_t value, bool selected)
{
    /* Operation order is funny because this is more optimized :-) */
    char temp[9] = {'\0','\0','\0','\0','\0','\0','\0','\0','\0'};

    if (selected == true)
    {
        /* arrow symbol */
        temp[0] = 0x7E;
    }

    temp[1] = ' ';
    temp[3] = '.';

    if (type == PSU_SETPOINT_VOLTAGE)
    {

        temp[5] = get_digit(&value);
        temp[4] = get_digit(&value);
        temp[2] = get_digit(&value);

        /* Display 10s if voltage setpoint selected */
        value /= 10U;
        temp[1] = get_digit(&value);

        temp[6] = 'V';
        temp[7] = ' ';
    }
    else
    {

        temp[6] = get_digit(&value);
        temp[5] = get_digit(&value);
        temp[4] = get_digit(&value);
        temp[2] = get_digit(&value);

        temp[7] = 'A';

    }

    temp[8] = 0;
    display_write_string(temp);

}

static void gui_main_screen(void)
{

    bool selected;

    /* Line 1 */
    display_set_cursor(0, 0);
    display_write_char('V');

    selected = (application.selected_psu == PSU_CHANNEL_0 && application.selected_setpoint == PSU_SETPOINT_VOLTAGE) ? true : false;
    gui_print_measurement(PSU_SETPOINT_VOLTAGE, psu_channels[PSU_CHANNEL_0].voltage_setpoint.value.raw, selected);

    selected = (application.selected_psu == PSU_CHANNEL_1 && application.selected_setpoint == PSU_SETPOINT_VOLTAGE) ? true : false;
    gui_print_measurement(PSU_SETPOINT_VOLTAGE, psu_channels[PSU_CHANNEL_1].voltage_setpoint.value.raw, selected);
    display_write_char(psu_channels[PSU_CHANNEL_0].state == PSU_STATE_OPERATIONAL ? '1' : '0');
    /* Line 2 */
    display_set_cursor(1, 0);
    display_write_char('I');

    selected = (application.selected_psu == PSU_CHANNEL_0 && application.selected_setpoint == PSU_SETPOINT_CURRENT) ? true : false;
    gui_print_measurement(PSU_SETPOINT_CURRENT, psu_channels[PSU_CHANNEL_0].current_setpoint.value.raw, selected);

    selected = (application.selected_psu == PSU_CHANNEL_1 && application.selected_setpoint == PSU_SETPOINT_CURRENT) ? true : false;
    gui_print_measurement(PSU_SETPOINT_CURRENT, psu_channels[PSU_CHANNEL_1].current_setpoint.value.raw, selected);

    if (psu_channels[PSU_CHANNEL_1].state == PSU_STATE_SAFE_STATE)
        display_write_char('S');
    else
        display_write_char(psu_channels[PSU_CHANNEL_1].state == PSU_STATE_OPERATIONAL ? '1' : '0');
}

static e_psu_gui_menu psu_menu_handler(e_psu_gui_menu page)
{

    e_key_event evt;		/* event from the input system */
    e_menu_event menu_evt;	/* event to be fed to the menu system */
    e_psu_gui_menu new_page;	/* newly selected page if any */

    new_page = page;
    evt = keypad_clicked(BUTTON_SELECT);

    switch(evt)
    {
    case KEY_CLICK:
        menu_evt = MENU_EVENT_CLICK;
        break;
    case KEY_HOLD:
        menu_evt = MENU_EVENT_CLICK_LONG;
        break;
    default:
        menu_evt = MENU_EVENT_NONE;
        break;
    }

    switch(page)
    {
    case PSU_MENU_PSU:

        //menu_set(NULL, NULL, 0U);
        if (evt == KEY_CLICK)
        {
            psu_advance_selection();
        }
        else if (evt == KEY_HOLD)
        {
        	new_page = PSU_MENU_MAIN;
        }

        gui_main_screen();

        break;
    case PSU_MENU_MAIN:
        //menu_set(&menu_state, &menu_item[0], 4U);
        if (evt == KEY_HOLD)
        {
        	new_page = PSU_MENU_PSU;
        }
        break;
    default:
        /* error; back to start */
        page = PSU_MENU_PSU;
        break;
    }

    if (new_page != page)
    {
    	/* Page changed, do initialization if necessary */
    }

    /* periodic function for the menu handler */
    menu_display();
    menu_event(menu_evt);

    return new_page;
}

/*
psu_channels[0].voltage_setpoint.value.raw = 19856;
observed an offset error of about 50mV
note that the prototype breadboard does not have a separatly filtered and regulated 5V supply
psu_channels[0].current_setpoint.value.raw = 1500;// = 2047; // observed an offset error of about 40mV
*/
// to-do / to analyze: 1) absolute offset calibration
//                     2) non linear behaviour correction (do measurements)

void psu_app_init(void)
{
    /* System init */
    system_init();

    /* Initialize I/Os */
    init_io();

    /* Init ranges and precisions */
    psu_init();

    /* Default state for the display */
    display_clear_all();
    display_enable_cursor(false);

    /* ...splash screen (busy wait, changed later) */
    display_write_string("IlLorenz");
    display_set_cursor(1, 0);
    display_write_string("PSU AVR");
    display_write_string((application.master_or_slave == TRUE) ? " (MASTER)" : " (SLAVE)");

    display_periodic();     /* call it at least once to clear the display */

    timer_delay_ms(2000);   /* splashscreen! */

    display_clear_all();
   // DBG_CONFIG;
    /* 10 kHz PID routine handler */
//    OCR0B = 200;

    /* enable output compare match interrupt on timer B */
 //   TIMSK0 |= (1 << OCIE0B);

}

//ISR(TIMER0_COMPB_vect)
//{
//    uint8_t i;
//DBG_HIGH;
//    for (i = 0; i < (uint8_t)PSU_CHANNEL_NUM; i++)
//    {
//        if (psu_channels[i].remote_or_local == false)
//        {
//            /* Local channel */
//
//            /* ADC readout */
//            psu_adc_processing(&psu_channels[i]);
//
//            /* PID controller */
//            psu_channels[i].current_setpoint.value.scaled = pid_Controller(psu_channels[i].current_setpoint.value.scaled,
//                                                                           psu_channels[i].current_readout.value.raw,
//                                                                           &psu_channels[i].current_limit_pid);
//
//            /* PWM */
//            psu_pwm_processing(&psu_channels[i]);
//
//        }
//        else
//        {
//            /* not timer critical -> handled in the low priority thread */
//        }
//    }
//    DBG_LOW;
//}

__attribute__((always_inline)) void inline psu_app(void)
{

    static volatile uint32_t start;

    start = g_timestamp;
    /* Periodic functions */
    psu_input_processing();

    /* Keypad */
    keypad_periodic(g_timestamp);

    /* GUI */
    menu_page = psu_menu_handler(menu_page);

    /* Output processing */
    psu_output_processing();

    /* Display handler */
    display_periodic();

    /* Send out the oldest datagram from the FIFO */
    datagram_buffer_to_remote();

#ifdef TIMER_DEBUG
    /* Debug the timer */
    timer_debug();
#endif

    application.cycle_time = g_timestamp - start;

}
