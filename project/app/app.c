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

static t_psu_channel psu_channels[PSU_CHANNEL_NUM];

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
static e_psu_gui_menu menu_page;
static uint8_t bool_values[2] = { 1, 0 };
static char* bool_labels[2] = { "YES", "NO" };
static t_menu_extra_list menu_extra_3 = { 2U, 0U, bool_labels, bool_values };
uint16_t cfg_ads1015;
uint16_t conv_ads1015;
uint16_t conv_ads1015_isr;
uint16_t duty;
uint16_t tmo_cnt = 0;
uint16_t dac_val = 0;

#define MAX(x, y) ((((x) > (y)) ? (x) : (y)))

const PROGMEM t_menu_item menu_test_eeprom[] = {
        {"dac", (void*)&dac_val, MENU_TYPE_NUMERIC_16 },
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

const PROGMEM t_menu_item menu_psu_ch_debug[] = {
        {"M/S", (void*)&application.master_or_slave, MENU_TYPE_NUMERIC_8 },
        {"c.t", (void*)&application.cycle_time, MENU_TYPE_NUMERIC_32 },
        {"c.t.m", (void*)&application.cycle_time_max, MENU_TYPE_NUMERIC_32 },
        {"BACK", NULL, MENU_TYPE_GOTO }
};

#define MAX_MENU_ENTRIES  ( \
        MAX(MAX(sizeof(menu_test_eeprom), sizeof(menu_test_debug)), sizeof(menu_psu_ch_debug)) \
        )

static t_menu_item g_megnu_page_entries[MAX_MENU_ENTRIES / sizeof(t_menu_item)];

static void uart_received(uint8_t byte)
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

    /* Buffers, FIFOs, ... */
    fifo_init(&remote_dgram_stream_fifo, remote_dgram_stream_buffer, REMOTE_DGRAM_BUF_SIZE);

    system_interrupt_enable();

#ifdef __AVR_ARCH__
    /*  */
    memcpy_P(g_megnu_page_entries, menu_test_eeprom, sizeof(menu_test_eeprom));
#else
    memcpy(g_megnu_page_entries, menu_test_eeprom, sizeof(menu_test_eeprom));
#endif
    ads_init();
    mcp_dac_init();
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
    remote_decode_datagram(psu_channels);

    /* Refresh Power Supply Data */

#warning "Test onlz!!"

//    cfg_ads1015 = ads_get_config();

    uint16_t tmp;

    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        tmp = conv_ads1015_isr;
    }

    /* Voltage */
    psu_set_measurement(&psu_channels[PSU_CHANNEL_0].voltage_readout, tmp);
    low_pass_filter(tmp, &psu_channels[PSU_CHANNEL_0].voltage_readout.filter);
    psu_set_measurement(&psu_channels[PSU_CHANNEL_0].voltage_readout,
                         psu_channels[PSU_CHANNEL_0].voltage_readout.filter.output);

    /* Current */
    psu_set_measurement(&psu_channels[PSU_CHANNEL_0].current_readout, tmp);
    low_pass_filter(tmp, &psu_channels[PSU_CHANNEL_0].current_readout.filter);
    psu_set_measurement(&psu_channels[PSU_CHANNEL_0].current_readout,
                         psu_channels[PSU_CHANNEL_0].current_readout.filter.output);

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

    /* Init remote protocol */
    remote_init();

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

#ifdef __AVR_ARCH__
    /* 10 kHz PID routine handler */
    OCR0B = 200;

    /* enable output compare match interrupt on timer B */
    TIMSK0 |= (1 << OCIE0B);
#endif
}

#ifdef __AVR_ARCH__
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
        if (temp > 4095) temp = 4095;
        if (temp < 0) temp = 0;
        p = (uint16_t)temp;
        mcp_dac_write(p);

        isr_timer = 0;
    }
    else
    {
        /* Measure the voltage */

        psu_channels[0].voltage_readout.value.raw = conv_ads1015;
    }

    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        atomic_lock = FALSE;
    }


}
#endif

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
    remote_periodic(application.flag_50ms.flag);

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

#ifdef TIMER_DEBUG
    /* Debug the timer */
    timer_debug();
#endif

    application.cycle_time = g_timestamp - start;
    application.cycle_time /= 1000;
    if (application.cycle_time > application.cycle_time_max)
        application.cycle_time_max = application.cycle_time;

}
