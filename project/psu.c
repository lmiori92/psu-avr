/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty ofads_get_config
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Lorenzo Miori (C) 2016 [ 3M4|L: memoryS60<at>gmail.com ]

    Version History
        * 1.0 initial

*/

#include <util/atomic.h>

#include "HAL/inc/adc.h"
#include "HAL/inc/encoder.h"
#include "i2c/i2c_master.h"
#include "HAL/driver/adc/ads1015.h"
#include "HAL/inc/pwm.h"
#include "psu.h"
#include "HAL/inc/uart.h"
#include "remote.h"
#include "settings.h"
#include "HAL/inc/system.h"
#include "HAL/inc/time_m.h"

/* Standard Library */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* External Library */
#include "deasplay/deasplay.h"
#include "lorenzlib/lib.h"
#include "megnu/menu.h"
#include "keypad/keypad.h"

/* DEFINES */
#define ENCODER_TICK_MAX            20U
#define ENCODER_EVENT_QUEUE_LEN     (10U)

/* CONSTANTS */
const uint8_t psu_channel_node_id_map[PSU_CHANNEL_NUM][2] =
{
        /* MASTER - SLAVE (0: local channel or disabled) */
        { 0, 1 },
        { 1, 0 }
};

/* GLOBALS */

static bool encoder_menu_mode = false;

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

typedef struct
{
    e_enc_event event;
    uint8_t    delta;
} t_encoder_event_entry;

static uint8_t queue_index = 0U;
static t_encoder_event_entry encoder_event_queue[ENCODER_EVENT_QUEUE_LEN];

/* Remote application level buffers */
static t_remote_datagram_buffer remote_dgram_rcv_copy;

/* MENU (testing) */
static e_psu_gui_menu menu_page;
static uint8_t bool_values[2] = { 1, 0 };
static char* bool_labels[2] = { "YES", "NO" };
static t_menu_extra_list menu_extra_3 = { 2U, 0U, bool_labels, bool_values };
uint16_t cfg_ads1015;
uint16_t conv_ads1015;
uint16_t conv_ads1015_isr;
uint16_t duty;
uint16_t tmo_cnt = 0;

#include "avr/eeprom.h"     /* EEMEM definitions   */
#include "avr/pgmspace.h"   /* PROGMEM definitions */

const PROGMEM t_menu_item menu_test_eeprom[] = {
        {"cfg", (void*)&cfg_ads1015, MENU_TYPE_NUMERIC_16 },
        {"conv", (void*)&conv_ads1015, MENU_TYPE_NUMERIC_16 },
        {"set", (void*)&tmo_cnt, MENU_TYPE_NUMERIC_16 },
        {"P", (void*)&psu_channels[0].current_limit_pid.P_Factor, MENU_TYPE_NUMERIC_16 },
                                    {"I", (void*)&psu_channels[0].current_limit_pid.I_Factor, MENU_TYPE_NUMERIC_16 },
                                    {"D", (void*)&psu_channels[0].current_limit_pid.D_Factor, MENU_TYPE_NUMERIC_16 },
                                    {"Isr", (void*)&psu_channels[0].current_setpoint.value.raw, MENU_TYPE_NUMERIC_16 },
                                    {"Iss", (void*)&psu_channels[0].current_setpoint.value.scaled, MENU_TYPE_NUMERIC_16 },
                                    {"Vss", (void*)&psu_channels[0].voltage_setpoint.value.scaled, MENU_TYPE_NUMERIC_16 },
                                    {"Vsr", (void*)&psu_channels[0].voltage_setpoint.value.raw, MENU_TYPE_NUMERIC_16 },

                                    {"Irr", (void*)&psu_channels[0].current_readout.value.raw, MENU_TYPE_NUMERIC_16 },
                                    {"Irs", (void*)&psu_channels[0].current_readout.value.scaled, MENU_TYPE_NUMERIC_16 },
                                    {"Vrs", (void*)&psu_channels[0].voltage_readout.value.scaled, MENU_TYPE_NUMERIC_16 },
                                    {"Vrr", (void*)&psu_channels[0].voltage_readout.value.raw, MENU_TYPE_NUMERIC_16 },
                                    {"duty", (void*)&duty, MENU_TYPE_NUMERIC_16 },

                                   {"d.I", (void*)&psu_channels[0].current_limit_pid.sumError, MENU_TYPE_NUMERIC_32 },
                                    {"c.t", (void*)&application.cycle_time, MENU_TYPE_NUMERIC_32 },
                                    {"c.t.m", (void*)&application.cycle_time_max, MENU_TYPE_NUMERIC_32 },
                                    /*{"PID", (void*)&menu_extra_3, MENU_TYPE_LIST },*/
                                    {"BACK", (void*)(uint8_t)PSU_MENU_PSU, MENU_TYPE_GOTO }
                                    };

const PROGMEM t_menu_item menu_test_debug[] = {
        {"d.I", (void*)&psu_channels[0].current_limit_pid.sumError, MENU_TYPE_NUMERIC_32 },
        {"c.t", (void*)&application.cycle_time, MENU_TYPE_NUMERIC_32 },
        {"c.t.m", (void*)&application.cycle_time_max, MENU_TYPE_NUMERIC_32 },
        {"BACK", NULL, MENU_TYPE_GOTO }
};

static t_menu_item g_megnu_page_entries[20];

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
    channel->current_readout.scale.max_scaled = 2200;     /* Current */

    channel->voltage_setpoint.scale.min = 0;
    channel->voltage_setpoint.scale.max = 28500;
    channel->voltage_setpoint.scale.min_scaled = 0;
    channel->voltage_setpoint.scale.max_scaled = pwm_get_resolution(channel->voltage_pwm_channel);

    channel->current_setpoint.scale.min = 0;
    channel->current_setpoint.scale.max = 2200;
    channel->current_setpoint.scale.min_scaled = 0;
    channel->current_setpoint.scale.max_scaled = pwm_get_resolution(channel->current_pwm_channel);

    pid_Init(10,25,10, &channel->current_limit_pid);
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

/**
 * This is the encoder callback. Warning: do not overload it
 * e.g. store the events in a small queue and process the events
 * in the/a main/low priority thread.
 *
 * @param event         the encoder event, e_enc_event
 * @param delta_t       the number of ticks the event needed to be triggered
 */
static void encoder_event_callback(e_enc_event event, uint8_t ticks)
{
    if (queue_index < ENCODER_EVENT_QUEUE_LEN)
    {
        encoder_event_queue[queue_index].event = event;
        encoder_event_queue[queue_index].delta = ticks;
        queue_index++;
    }
    else
    {

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
    display_init();

    /* Encoder */
    encoder_init();
    encoder_set_callback(ENC_HW_0, encoder_event_callback);

    /* Keypad */
    keypad_init();

    /* I2C bus and peripherals */
    i2c_init();

    /* Debug pin */
    DBG_CONFIG;

    system_interrupt_enable();

    /*  */
    memcpy_P(g_megnu_page_entries, menu_test_eeprom, sizeof(menu_test_eeprom));

    ads_init();
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
    uint8_t j;

    /* Parse encoder events */
    uint16_t diff = 0U;

    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        /* no more event is added in the meantime... */
        queue_index = 0xFFU;
    }
    e_enc_event evt;
    for (i = 0; i < ENCODER_EVENT_QUEUE_LEN; i++)
    {

        evt = encoder_event_queue[i].event;

        if (evt == ENC_EVT_NUM)
        {
            /* Skip - Placeholder */
        }
        else if (evt == ENC_EVT_CLICK_DOWN)
        {
            /* Click event */
            keypad_set_input(BUTTON_SELECT, false);
        }
        else if (evt == ENC_EVT_CLICK_UP)
        {
            /* Release event */
            keypad_set_input(BUTTON_SELECT, true);
        }
        else if (evt != ENC_EVT_NUM)
        {
            /* Rotation event */
    //        for (j = 0; j < SMOOTHING_SIZE; j++)
    //        {
    //            if (encoder_event_queue[i].delta >= smoothing_deltat[j])
    //            {
    //                diff = smoothing_result[j];
    //                break;
    //            }
    //        }

            diff = ENCODER_TICK_MAX - encoder_event_queue[i].delta;//smoothing_result[encoder_event_queue[i].delta];

            if (encoder_menu_mode == TRUE)
            {
                menu_set_diff(diff);
                /* atomic read of the flag: menu mode */
                if (evt == ENC_EVT_LEFT) menu_event(MENU_EVENT_LEFT);
                else if (evt == ENC_EVT_RIGHT) menu_event(MENU_EVENT_RIGHT);
            }
            else
            {

                if (evt == ENC_EVT_LEFT)
                {
                    lib_diff(&application.selected_setpoint_ptr->value.raw, diff);
                }
                else if (evt == ENC_EVT_RIGHT)
                {
                    lib_sum(&application.selected_setpoint_ptr->value.raw, application.selected_setpoint_ptr->scale.max, diff);
                }

            }
        }

        encoder_event_queue[i].event = ENC_EVT_NUM;

    }

    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        queue_index = 0;
    }

    /* Parse remote datagrams */
    remote_decode_datagram();

    for (i = 0; i < (uint8_t)PSU_CHANNEL_NUM; i++)
    {
        if (psu_channels[i].remote_or_local == false)
        {
            /* Local channel */
            psu_adc_processing(&psu_channels[i]);
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
//        if (psu_channels[i].state == PSU_STATE_OPERATIONAL)
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
                    //remote_encode_datagram(DATATYPE_SETPOINTS, &psu_channels[i]);
                }
                else
                {
                    /* Slave(s) are not sending setpoint values */
                }
            }
        }
    }

}

static char get_digit(uint16_t *val)
{
    *val = *val / 10U;
    return '0' + *val % 10U;
}

static void gui_print_measurement(e_psu_setpoint type, uint16_t value, bool selected)
{
    /* Operation order is funny because this is more optimized :-) */
    char temp[8];

    if (selected == true)
    {
        /* arrow symbol */
        temp[0] = 0x7E;
    }
    else
    {
        temp[0] = ' ';
    }

    if (type == PSU_SETPOINT_VOLTAGE)
    {
        temp[6] = 'V';
    }
    else
    {
        temp[6] = 'A';
    }

    /* digits */
    temp[5] = get_digit(&value);
    temp[4] = get_digit(&value);
    temp[3] = '.';
    temp[2] = get_digit(&value);
    temp[1] = get_digit(&value);

    temp[7] = '\0';
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
    e_menu_input_event menu_evt;	    /* event to be fed to the menu system */
    e_menu_output_event menu_evt_out;    /* event generated by the menu system */
    e_psu_gui_menu new_page;	/* newly selected page if any */
    t_menu_item  *menu_item = NULL;
    uint8_t       menu_count = 0U;
    static t_menu_state menu_state = {0, 1, MENU_NOT_SELECTED, 1U};

    new_page = page;
    evt = keypad_clicked(BUTTON_SELECT);

    /* Input event to Menu event mapping */
    /* The Left and Right events are handled in the interrupt */
    if (evt == KEY_CLICK) menu_evt = MENU_EVENT_CLICK;
    else if (evt == KEY_HOLD) menu_evt = MENU_EVENT_CLICK_LONG;
    else menu_evt = MENU_EVENT_NONE;

    menu_evt_out = menu_event(menu_evt);

    switch(page)
    {
    case PSU_MENU_PSU:
        if (evt == KEY_CLICK)
        {
            psu_advance_selection();
        }
        else if (evt == KEY_HOLD)
        {
        	new_page = PSU_MENU_MAIN;
        }
        else if (menu_evt_out == MENU_EVENT_OUTPUT_GOTO)
        {
            new_page = PSU_MENU_MAIN;
        }

        gui_main_screen();

        break;
    case PSU_MENU_MAIN:

        encoder_menu_mode = true;

        if (evt == KEY_CLICK)
        {
            application.cycle_time_max = 0;
        }
        else if (evt == KEY_HOLD)
        {
            new_page = PSU_MENU_PSU;
        }

        if (menu_evt_out == MENU_EVENT_OUTPUT_EXTRA_EDIT)
        {
//            pid_Init(psu_channels[0].current_limit_pid.P_Factor, psu_channels[0].current_limit_pid.I_Factor, psu_channels[0].current_limit_pid.D_Factor, &psu_channels[0].current_limit_pid);
//            pid_Reset_Integrator(&psu_channels[0].current_limit_pid);
        }
        else if (menu_evt_out == MENU_EVENT_OUTPUT_GOTO)
        {
            new_page = PSU_MENU_PSU;
        }

        break;
    case PSU_MENU_STARTUP:
        /* initialization phase */
        new_page = PSU_MENU_MAIN;
        break;
    default:
        /* error or PSU_MENU_STARTUP */
        break;
    }

    if (new_page != page)
    {
    	/* Page changed, do initialization if necessary */
        switch(new_page)
        {
        case PSU_MENU_PSU:
            encoder_menu_mode = false;
            break;
        case PSU_MENU_MAIN:
            encoder_menu_mode = true;
            /* select the menu */
            menu_item  = &g_megnu_page_entries[0];
            menu_count = (sizeof(g_megnu_page_entries) / sizeof(t_menu_item));
            break;
        default:
            break;
        }

        menu_set(&menu_state, menu_item, menu_count, (uint8_t)page);
    }
    else
    {
        /* no page switch */
    }

    /* periodic function for the menu handler */
    menu_display();

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
    display_clear();
    display_enable_cursor(false);

    /* Settings (parameters) */
    settings_init();

    /* Read the data out the persistent storage */
    settings_read_from_storage();

    if (setting_has_property(SETTING_VERSION, SETTING_STATE_VALID) == true)
    {
        // testing
        setting_set_1(SETTING_VERSION, setting_get_1(SETTING_VERSION, 0U) + 1);
        settings_save_to_storage(SETTING_NUM_SETTINGS);
    }
    else
    {
        // no valid storage : do not save yet!
        setting_set_1(SETTING_VERSION, 0xFF);
        settings_save_to_storage(SETTING_NUM_SETTINGS);
    }

    /* ...splash screen (busy wait, changed later) */
    display_write_string("IlLorenz");
    display_set_cursor(1, 0);
    display_write_string("PSU AVR");
    display_write_string((application.master_or_slave == TRUE) ? " (MASTER)" : " (SLAVE)");

    display_periodic();     /* call it at least once to clear the display */

    timer_delay_ms(2000);   /* splashscreen! */

    display_clear();

    /* start with the following menu page */
    menu_page = PSU_MENU_STARTUP;

   // DBG_CONFIG;
    /* 10 kHz PID routine handler */
    OCR0B = 200;

    /* enable output compare match interrupt on timer B */
    TIMSK0 |= (1 << OCIE0B);

}

ISR(TIMER0_COMPB_vect)
{
    static uint8_t isr_timer;
    static bool atomic_lock;

    if (atomic_lock == TRUE)
    {
        return;
    }

    atomic_lock = TRUE;

    /* immediately re-enable interrupts */
    sei();

    isr_timer++;
    if (isr_timer > 5)
    {
        encoder_tick(ENCODER_TICK_MAX);
        /* execute logic at 1khz */
        conv_ads1015_isr = ads_read();
//        uint8_t state = i2c_get_state_info();
//        if (state == TWI_TIMEOUT)
//        {
//            tmo_cnt++;
//        }
        conv_ads1015_isr >>= 4; // bit lost in single ended conversion!

        int16_t temp;
        int16_t sp = tmo_cnt;
        int16_t fb = conv_ads1015_isr;
        uint16_t p;
        temp = pid_Controller(sp, fb, &psu_channels[0].current_limit_pid);
        if (temp > 0x3FF) temp = 0x3FF;
        if (temp < 0) temp = 0;
        p = (uint16_t)temp;
        pwm_set_duty(PWM_CHANNEL_0, p);

        isr_timer = 0;
    }

    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        atomic_lock = FALSE;
    }


}
//
//    uint16_t adc_cur;
//    adc_cur = adc_get(ADC_1);
//
//    /* ADC readout */
//    psu_channels[0].current_readout.value.raw = adc_cur;
//
//    /* PID controller */
//    int16_t temp;
//    temp = pid_Controller(150,
//                                                                   adc_cur,
//                                                                   &psu_channels[0].current_limit_pid);
//
//    if (temp > 0x3FF) { temp = 0x3FF; pid_Reset_Integrator(&psu_channels[0].current_limit_pid); }
//    if (temp < 0) { temp = 0; pid_Reset_Integrator(&psu_channels[0].current_limit_pid); }
//
//    duty = (uint16_t)temp;
//    /* PWM */
//    pwm_set_duty(PWM_CHANNEL_0, duty);
//
//}

void psu_app(void)
{

    static volatile uint32_t start;

    start = g_timestamp;

    /* check 50ms flag */
    application.flag_50ms.flag = false;
    if ((g_timestamp - application.flag_50ms.timestamp) > KEY_50MS_FLAG)
    {
        /* 50ms elapsed, set the flag */
        application.flag_50ms.flag = true;
        application.flag_50ms.timestamp = g_timestamp;
    }
    else
    {
        /* waiting for the 50ms flag */
    }

    /* Periodic functions */
    psu_input_processing();

    /* Keypad */
    keypad_periodic(application.flag_50ms.flag);

    /* GUI */
    menu_page = psu_menu_handler(menu_page);

    /* Output processing */
    psu_output_processing();

    /* Display handler */
    display_periodic();

    /* Send out the oldest datagram from the FIFO */
    datagram_buffer_to_remote();


#warning "Test onlz!!"

//    cfg_ads1015 = ads_get_config();


    static t_low_pass_filter flt;
    uint16_t tmp;
    flt.alpha = 100;
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        tmp = conv_ads1015_isr;
    }
    low_pass_filter(tmp, &flt);
    conv_ads1015 =     (uint32_t)flt.output;

#ifdef TIMER_DEBUG
    /* Debug the timer */
    timer_debug();
#endif

    application.cycle_time = g_timestamp - start;
    application.cycle_time /= 1000;
    if (application.cycle_time > application.cycle_time_max)
        application.cycle_time_max = application.cycle_time;

}
