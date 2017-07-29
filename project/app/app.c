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

#ifdef __AVR_ARCH__
/* TODO if possible, remove the architecture related ifdefs ! */
#include <util/atomic.h>    /* Atomic operation macros */
#include "avr/pgmspace.h"   /* PROGMEM definitions */
#else
#define PROGMEM
#define ATOMIC_BLOCK(x)
#endif

#include "app.h"
#include "psu.h"
#include "inc/adc.h"
#include "inc/encoder.h"
#include "driver/adc/ads1015.h"
#include "driver/dac/mcp4725.h"
#include "inc/pwm.h"
#include "inc/uart.h"
#include "inc/system.h"
#include "inc/time_m.h"
#include "inc/i2c_master.h"

#include "remote.h"
#include "settings.h"

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

/** The maximum tick count before acknowledging encoder timeout */
#define ENCODER_TICK_MAX            20U
/** The maximum number of encoder events in a single low-priority task */
#define ENCODER_EVENT_QUEUE_LEN     (10U)

/* @9600BAUD -> (9600 / 10 / 8) = 120 byte/s ~9ms per byte, so, at every
 * cycle, assuming and absulte maximum (10-20x times the normal one),
 * 10 bytes buffer is more than sufficient */
/** The number of characters that are processed in a single low-priority task cycle */
#define REMOTE_DGRAM_BUF_SIZE       (10U)

/* GLOBALS */

static bool encoder_menu_mode = false;

#define PSU_NUM_CHANNELS        2U
static t_psu_channel psu_channels[PSU_NUM_CHANNELS];

static t_application application;

typedef struct
{
    e_enc_event event;
    uint8_t    delta;
} t_encoder_event_entry;

static uint8_t queue_index = 0U;
static t_encoder_event_entry encoder_event_queue[ENCODER_EVENT_QUEUE_LEN];

/* Remote application level buffers */
static uint8_t remote_dgram_stream_buffer[REMOTE_DGRAM_BUF_SIZE];
static t_fifo remote_dgram_stream_fifo;

/* MENU (testing) */
static uint8_t bool_values[2] = { 1, 0 };
static char* bool_labels[2] = { "YES", "NO" };
static t_menu_extra_list menu_extra_3 = { 2U, 0U, bool_labels, bool_values };
uint16_t tmo_cnt = 0;

#define MAX(x, y) ((((x) > (y)) ? (x) : (y)))

const PROGMEM t_menu_item menu_test_eeprom[] = {
    {"VOLTAGE ADC CAL", (void*)(uint8_t)PSU_MENU_VOLTAGE_CALIBRATION, MENU_TYPE_GOTO },
    {"CURRENT ADC CAL", (void*)(uint8_t)PSU_MENU_CURRENT_CALIBRATION, MENU_TYPE_GOTO },
    {"VOLTAGE DAC CAL", (void*)(uint8_t)PSU_MENU_VOLTAGE_DAC_CALIBRATION, MENU_TYPE_GOTO },
    {"CURRENT DAC CAL", (void*)(uint8_t)PSU_MENU_CURRENT_DAC_CALIBRATION, MENU_TYPE_GOTO },

    {"Isr", (void*)&psu_channels[0].current_setpoint.value.raw, MENU_TYPE_NUMERIC_16 },
    {"Iss", (void*)&psu_channels[0].current_setpoint.value.scaled, MENU_TYPE_NUMERIC_16 },
    {"Vss", (void*)&psu_channels[0].voltage_setpoint.value.scaled, MENU_TYPE_NUMERIC_16 },
    {"Vsr", (void*)&psu_channels[0].voltage_setpoint.value.raw, MENU_TYPE_NUMERIC_16 },

//                                    {"c.t", (void*)&application.cycle_time, MENU_TYPE_NUMERIC_32 },
//                                    {"c.t.m", (void*)&application.cycle_time_max, MENU_TYPE_NUMERIC_32 },
                                    {"BACK", (void*)(uint8_t)PSU_MENU_PSU, MENU_TYPE_GOTO }
                                    };

const PROGMEM t_menu_item menu_test_debug[] = {
        //        {"d.I", (void*)&psu_channels[0].current_limit_pid.sumError, MENU_TYPE_NUMERIC_32 },
        {"c.t", (void*)&application.cycle_time, MENU_TYPE_NUMERIC_32 },
        {"c.t.m", (void*)&application.cycle_time_max, MENU_TYPE_NUMERIC_32 },
        {"BACK", NULL, MENU_TYPE_GOTO }
};

const PROGMEM t_menu_item menu_psu_ch_debug[] = {
        {"M/S", (void*)&application.master_or_slave, MENU_TYPE_NUMERIC_8 },
        {"c.t", (void*)&application.cycle_time, MENU_TYPE_NUMERIC_32 },
        {"c.t.m", (void*)&application.cycle_time_max, MENU_TYPE_NUMERIC_32 },
        {"BACK", NULL, MENU_TYPE_GOTO }
};

//#define MAX_MENU_ENTRIES  ( \
//        MAX(MAX(sizeof(menu_test_eeprom), sizeof(menu_test_debug)), sizeof(menu_psu_ch_debug)) \
//        )

//static t_menu_item g_megnu_page_entries[MAX_MENU_ENTRIES / sizeof(t_menu_item)];

/** STATIC PROTOTYPES **/
static void app_gui_load_page(uint8_t page);

void uart_received(uint8_t byte)
{

    /* NOTE: interrupt callback. Pay attention to execution time... */

    fifo_push(&remote_dgram_stream_fifo, byte);

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

static void psu_init(void)
{

    uint8_t i;

    for (i = 0; i < PSU_NUM_CHANNELS; i++)
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
        /* buffer available */
        encoder_event_queue[queue_index].event = event;
        encoder_event_queue[queue_index].delta = ticks;
        queue_index++;
    }
    else
    {
        /* no more buffer */
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
    /* do not directly stdout, the remote protocol will break ! */
    /* stdout = &uart_output; */
    /* stdin  = &uart_input; */

    /* ADC */
    adc_init();

    /* PWM */
//    pwm_init();
//    pwm_enable_channel(PWM_CHANNEL_0);
//    pwm_enable_channel(PWM_CHANNEL_1);

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

    /* Buffers, FIFOs, ... */
    fifo_init(&remote_dgram_stream_fifo, remote_dgram_stream_buffer, REMOTE_DGRAM_BUF_SIZE);

    system_interrupt_enable();

    ads_init();
    mcp_dac_init(MCP_DAC_ADDRESS_0);
    mcp_dac_init(MCP_DAC_ADDRESS_1);
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
    /* Feedback works this way:
     * - injecting more current: voltage setpoint goes towards zero
     * - injecting less current: voltage setpoint goes towards Vmax
     * Therefore, this is translated here. The minimum setpoint (0V) is
     * obtained with a maximum DAC output and vice-versa */
    channel->voltage_setpoint.value.scaled = channel->voltage_setpoint.scale.max_scaled - channel->voltage_setpoint.value.scaled + channel->voltage_setpoint.scale.min_scaled;
    /* Current Scaling */
    lib_scale(&channel->current_setpoint.value, &channel->current_setpoint.scale);
}

static void psu_input_processing(void)
{

    uint8_t i;

    /* Parse encoder events */
    uint16_t diff;
    e_enc_event evt;

    uint8_t byte;
    bool fifo_result;

    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        /* no more event is added in the meantime... */
        queue_index = 0xFFU;
    }

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

            diff = ENCODER_TICK_MAX - encoder_event_queue[i].delta;

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

    /* Remote Datagram UART buffer */

    for (i = 0; i < REMOTE_DGRAM_BUF_SIZE; i++)
    {
        ATOMIC_BLOCK(ATOMIC_FORCEON)
        {
            fifo_result = fifo_pop(&remote_dgram_stream_fifo, &byte);
        }

        if (fifo_result == true)
        {
            /* Call the state machine with a single byte... */
            remote_buffer_to_datagram(byte);
        }
    }

    /* Parse remote datagrams */
    remote_decode_datagram(psu_channels, PSU_NUM_CHANNELS);

    /* Refresh Power Supply Data */

    //cfg_ads1015 = ads_get_config();

    uint16_t tmp;

//    ATOMIC_BLOCK(ATOMIC_FORCEON)
//    {
//        tmp = conv_ads1015_isr;
//    }
    static uint8_t cur_volt_switch = 0;
    switch(cur_volt_switch)
    {
    case 0:
        ads_select_channel(ADS1015_CHANNEL_0, ADS_ADC_PGA_4_096);
        _delay_ms(1);
        cur_volt_switch = 1;
        tmp = ads_read();
        /* Voltage */
        psu_set_measurement(&psu_channels[PSU_CHANNEL_0].voltage_readout, tmp);
        low_pass_filter(tmp, &psu_channels[PSU_CHANNEL_0].voltage_readout.filter);
        psu_set_measurement(&psu_channels[PSU_CHANNEL_0].voltage_readout,
                             psu_channels[PSU_CHANNEL_0].voltage_readout.filter.output);
        break;
    case 1:
        ads_select_channel(ADS1015_CHANNEL_1, ADS_ADC_PGA_0_256);
        _delay_ms(1);
        cur_volt_switch = 0;
        tmp = ads_read();
        /* Current */
        psu_set_measurement(&psu_channels[PSU_CHANNEL_0].current_readout, tmp);
        low_pass_filter(tmp, &psu_channels[PSU_CHANNEL_0].current_readout.filter);
        psu_set_measurement(&psu_channels[PSU_CHANNEL_0].current_readout,
                             psu_channels[PSU_CHANNEL_0].current_readout.filter.output);
        break;
    }

    /* Post-processing of Power Supply Channels */

    /* Perform checks on the channel */
    psu_check_channel(&psu_channels[PSU_CHANNEL_0], application.flag_50ms.flag);

    /* Post-processing (scaling) of the values */
    psu_postprocessing(&psu_channels[PSU_CHANNEL_0]);

    if (application.master_or_slave == TRUE)
    {
        /* Perform checks on the channel */
        psu_check_channel(&psu_channels[PSU_CHANNEL_1], application.flag_50ms.flag);
    }
    else
    {
        /* The slave device sends readouts to the master */
        remote_encode_datagram(DATATYPE_READOUTS, &psu_channels[PSU_CHANNEL_0]);
    }

}

static void psu_output_processing(void)
{

    /* Pre-processing (scaling) of the values */
    psu_preprocessing(&psu_channels[PSU_CHANNEL_0]);

    if (application.master_or_slave == TRUE)
    {
        /* Output controlling is performed in the high-priority task,
         * prepare here the datagram for the setpoints */
        remote_encode_datagram(DATATYPE_SETPOINTS, &psu_channels[PSU_CHANNEL_1]);
    }
    else
    {
        /* The slave device does not send anything but readouts */
    }

}

static char get_digit(uint16_t *val)
{
    *val = *val / 10U;
    return '0' + *val % 10U;
}

static void app_menu_calibration(uint16_t *low, uint16_t *upper, uint16_t *raw, uint16_t *scaled)
{
    t_menu_item temp;
    temp.type = MENU_TYPE_NUMERIC_16;
    menu_clear();
    temp.label = "LOWER";
    temp.extra = (void*)low;
    menu_item_add(&temp);
    temp.label = "UPPER";
    temp.extra = (void*)upper;
    menu_item_add(&temp);
    temp.label = "RAW";
    temp.extra = (void*)raw;
    temp.type = MENU_TYPE_NUMERIC_16_EDIT;
    menu_item_add(&temp);
    temp.label = "SCALED";
    temp.extra = (void*)scaled;
    temp.type = MENU_TYPE_NUMERIC_16;
    menu_item_add(&temp);
}

static void app_set_page(const t_menu_item *page, uint8_t count)
{
    uint8_t i = 0;

    for (i = 0; i < count; i++)
    {
        menu_item_add(page + i);
    }
}

static void app_set_page_progmem(const PROGMEM t_menu_item menu[], uint8_t count)
{
    uint8_t i = 0;
    t_menu_item item;

    for (i = 0; i < count; i++)
    {
        memcpy_P(&item, menu + i, sizeof(item));
        menu_item_add(&item);
    }
}

static void app_gui_init(void)
{
    menu_init(2U);  /* lines */
    menu_set_page(PSU_MENU_MAIN);
    menu_set_diff(1);
    app_gui_load_page(menu_get_page());
}

static void app_gui_load_page(uint8_t page)
{
    menu_clear();
    t_menu_item back = {"BACK", (void*)(uint8_t)PSU_MENU_PSU, MENU_TYPE_GOTO };

    encoder_menu_mode = true;
    psu_channels[0].output_bypass = false;

    switch(page)
    {
    case PSU_MENU_MAIN:
        app_set_page_progmem(menu_test_eeprom, sizeof(menu_test_eeprom)/sizeof(menu_test_eeprom[0]));
        break;
    case PSU_MENU_PSU:
        encoder_menu_mode = false;
        menu_clear();
        break;
    case PSU_MENU_VOLTAGE_CALIBRATION:
        app_menu_calibration(&psu_channels[0].voltage_readout.scale.min,
                             &psu_channels[0].voltage_readout.scale.max,
                             &psu_channels[0].voltage_readout.value.raw,
                             &psu_channels[0].voltage_readout.value.scaled
                             );
        /* add a back button */
        back.extra = (void*)(uint8_t)PSU_MENU_MAIN;
        menu_item_add(&back);
        break;
    case PSU_MENU_CURRENT_CALIBRATION:
        app_menu_calibration(&psu_channels[0].current_readout.scale.min,
                             &psu_channels[0].current_readout.scale.max,
                             &psu_channels[0].current_readout.value.raw,
                             &psu_channels[0].current_readout.value.scaled
                             );
        /* add a back button */
        back.extra = (void*)(uint8_t)PSU_MENU_MAIN;
        menu_item_add(&back);
        break;
    case PSU_MENU_VOLTAGE_DAC_CALIBRATION:
        psu_channels[0].output_bypass = true;
        app_menu_calibration(&psu_channels[0].voltage_setpoint.scale.min_scaled,
                             &psu_channels[0].voltage_setpoint.scale.max_scaled,
                             &psu_channels[0].value_bypass,
                             &psu_channels[0].voltage_setpoint.value.scaled
                             );
        /* add a back button */
        back.extra = (void*)(uint8_t)PSU_MENU_MAIN;
        menu_item_add(&back);
        break;
    case PSU_MENU_CURRENT_DAC_CALIBRATION:
        psu_channels[0].output_bypass = true;
        app_menu_calibration(&psu_channels[0].current_setpoint.scale.min_scaled,
                             &psu_channels[0].current_setpoint.scale.max_scaled,
                             &psu_channels[0].current_setpoint.value.raw,
                             &psu_channels[0].current_setpoint.value.scaled
                             );
        /* add a back button */
        back.extra = (void*)(uint8_t)PSU_MENU_MAIN;
        menu_item_add(&back);
        break;
    default:
        break;
    }

    /* set the new page value for the application layer */
    menu_set_page(page);
}

static void save_calibration(e_settings_available setting, uint16_t *new, uint16_t *old)
{
    setting_set_2(setting, *new);
    settings_save_to_storage(setting);
    *old = *new;
}

void menu_event_callback(e_menu_output_event event, uint8_t index, uint8_t page, uint8_t info)
{
    switch(event)
    {
    case MENU_EVENT_OUTPUT_GOTO:
        /* select page */
        app_gui_load_page(info);
        break;
    case MENU_EVENT_CLICK_LONG:
        switch(page)
        {
        case PSU_MENU_MAIN:
            app_gui_load_page(PSU_MENU_PSU);
            break;
        default:
            break;
        }

        break;
    case MENU_EVENT_CLICK:
    case MENU_EVENT_OUTPUT_DESELECT:

        switch(page)
        {
        case PSU_MENU_MAIN:
            application.cycle_time_max = 0;
            break;
        case PSU_MENU_VOLTAGE_CALIBRATION:
            if (index == 0)
            {
                /* lower voltage calibration data save */
                save_calibration(SETTING_CAL_VOLTAGE_LOWER, &psu_channels[0].voltage_readout.value.raw, &psu_channels[0].voltage_readout.scale.min);
            }
            else if (index == 1)
            {
                /* upper voltage calibration data save */
                save_calibration(SETTING_CAL_VOLTAGE_UPPER, &psu_channels[0].voltage_readout.value.raw, &psu_channels[0].voltage_readout.scale.max);
            }
            break;
        case PSU_MENU_CURRENT_CALIBRATION:
            if (index == 0)
            {
                /* lower voltage calibration data save */
                save_calibration(SETTING_CAL_CURRENT_LOWER, &psu_channels[0].current_readout.value.raw, &psu_channels[0].current_readout.scale.min);
            }
            else if (index == 1)
            {
                /* upper voltage calibration data save */
                save_calibration(SETTING_CAL_CURRENT_UPPER, &psu_channels[0].current_readout.value.raw, &psu_channels[0].current_readout.scale.max);
            }
            break;
        case PSU_MENU_VOLTAGE_DAC_CALIBRATION:
            /* please note the values have to be inverted: it will be next implemented a generic inverted flag to a setpoint - measurement block */
            if (index == 0)
            {
                /* lower voltage calibration data save */
                save_calibration(SETTING_CAL_DAC_VOLTAGE_LOWER, &psu_channels[0].value_bypass, &psu_channels[0].voltage_setpoint.scale.max_scaled);
            }
            else if (index == 1)
            {
                /* upper voltage calibration data save */
                save_calibration(SETTING_CAL_DAC_VOLTAGE_UPPER, &psu_channels[0].value_bypass, &psu_channels[0].voltage_setpoint.scale.min_scaled);
            }
            break;
        case PSU_MENU_CURRENT_DAC_CALIBRATION:
            if (index == 0)
            {
                /* lower voltage calibration data save */
                save_calibration(SETTING_CAL_DAC_CURRENT_LOWER, &psu_channels[0].current_setpoint.value.scaled, &psu_channels[0].current_setpoint.scale.min_scaled);
            }
            else if (index == 1)
            {
                /* upper voltage calibration data save */
                save_calibration(SETTING_CAL_DAC_CURRENT_UPPER, &psu_channels[0].current_setpoint.value.scaled, &psu_channels[0].current_setpoint.scale.max_scaled);
            }
            break;
        default:
            break;
        }

        break;
    default:
        break;
    }
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

void psu_app_init(void)
{
    /* System init */
    system_init();

    /* Initialize I/Os */
    init_io();

    /* Settings (parameters) */
    settings_init();

    /* Read the data out the persistent storage */
    settings_read_from_storage();

    /* Init ranges and precisions */
    psu_init();

    /* Init remote protocol */
    remote_init();

    /* Default state for the display */
    display_clear();
    display_enable_cursor(false);

    /* ...splash screen (busy wait, changed later) */
    display_write_string("IlLorenz");
    display_set_cursor(1, 0);
    display_write_string("PSU AVR");
    display_write_string((application.master_or_slave == TRUE) ? " (MASTER)" : " (SLAVE)");

    display_periodic();     /* call it at least once to clear the display */

    timer_delay_ms(2000);   /* splashscreen! */

    display_clear();

    /* start with the following menu page */
    app_gui_init();
//
//#ifdef __AVR_ARCH__
//    /* 10 kHz PID routine handler */
//    OCR0B = 200;
//
//    /* enable output compare match interrupt on timer B */
//    TIMSK0 |= (1 << OCIE0B);
//#endif
}

//#ifdef __AVR_ARCH__
//ISR(TIMER0_COMPB_vect)
//{
//    static uint8_t isr_timer;
//    static bool atomic_lock;
//
//    if (atomic_lock == TRUE)
//    {
//        return;
//    }
//
//    atomic_lock = TRUE;
//
//    /* immediately re-enable interrupts */
//    sei();
//
//    isr_timer++;
//    if (isr_timer > 5)
//    {
//        encoder_tick(ENCODER_TICK_MAX);
//        /* execute logic at 1khz */
//        conv_ads1015_isr = ads_read();
////        uint8_t state = i2c_get_state_info();
////        if (state == TWI_TIMEOUT)
////        {
////            tmo_cnt++;
////        }
//        conv_ads1015_isr >>= 4; // bit lost in single ended conversion!
//
//
//
//        int16_t temp;
//        int16_t sp = tmo_cnt;
//        int16_t fb = conv_ads1015_isr;
//        uint16_t p;
//        temp = pid_Controller(sp, fb, &psu_channels[0].current_limit_pid);
//        if (temp > 4095) temp = 4095;
//        if (temp < 0) temp = 0;
//        p = (uint16_t)temp;
//        mcp_dac_write(p);
//
//        isr_timer = 0;
//    }
//    else
//    {
//        /* Measure the voltage */
//
//        psu_channels[0].voltage_readout.value.raw = conv_ads1015;
//    }
//
//    ATOMIC_BLOCK(ATOMIC_FORCEON)
//    {
//        atomic_lock = FALSE;
//    }
//
//
//}
//#endif

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

    /* Remote protocol */
    //remote_periodic(application.flag_50ms.flag);

    /* Periodic functions */
    psu_input_processing();

    /* Keypad */
    keypad_periodic(application.flag_50ms.flag);

    e_key_event evt = keypad_clicked(BUTTON_SELECT);
    e_menu_input_event menu_evt = MENU_EVENT_NONE;

    /* MENU */

    /* Input event to Menu event mapping */
    /* The Left and Right events are handled in the interrupt */
    if (evt == KEY_CLICK) menu_evt = MENU_EVENT_CLICK;
    else if (evt == KEY_HOLD) menu_evt = MENU_EVENT_CLICK_LONG;

    (void)menu_event(menu_evt);

    /* periodic function for the menu handler */
    menu_display();

    /* GUI */
    switch(menu_get_page())
    {
    case PSU_MENU_PSU:
        gui_main_screen();
        if (evt == KEY_CLICK)
        {
            psu_advance_selection();
        }
        else if (evt == KEY_HOLD)
        {
            app_gui_load_page(PSU_MENU_MAIN);
        }
        break;
    default:
        /* nothing to handle here */
        break;
    }

    /* Output processing */
    psu_output_processing();

    /* Display handler */
    display_periodic();

    if (psu_channels[0].output_bypass == false)
    {
        mcp_dac_write(MCP_DAC_ADDRESS_0, psu_channels[0].voltage_setpoint.value.scaled);
        mcp_dac_write(MCP_DAC_ADDRESS_1, psu_channels[0].current_setpoint.value.scaled);
    }
    else
    {
        /* outputs are bypassed */
        mcp_dac_write(MCP_DAC_ADDRESS_0, psu_channels[0].value_bypass);
    }

    /* Send out the oldest datagram from the FIFO */
    //datagram_buffer_to_remote();

#ifdef TIMER_DEBUG
    /* Debug the timer */
    timer_debug();
#endif

    application.cycle_time = g_timestamp - start;
    application.cycle_time /= 1000;
    if (application.cycle_time > application.cycle_time_max)
        application.cycle_time_max = application.cycle_time;

}
