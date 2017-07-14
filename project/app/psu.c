/*
 * psu.c
 *
 *  Created on: 02 ott 2016
 *      Author: lorenzo
 */

#include "psu.h"
#include "remote.h"

#include <stddef.h>

/* CONSTANTS */
static t_remote_datagram_buffer remote_dgram_rcv_copy;

void psu_init_channel(t_psu_channel *channel, e_psu_channel psu_ch, uint8_t identifier)
{

    /* Preoperational mode initially */
    channel->state             = PSU_STATE_INIT;

    /* Set nodeID */
    channel->node_id = identifier;

    psu_set_measurement_scale(&channel->voltage_readout, 0, 1777, 0, 20000);
    psu_set_measurement_scale(&channel->current_readout, 0, adc_get_resolution(), 0, 4300);

    psu_set_setpoint_scale(&channel->voltage_setpoint, 0, 25000, 487, mcp_dac_get_resolution());
    psu_set_setpoint_scale(&channel->current_setpoint, 0, 2048, 0, mcp_dac_get_resolution());

    /* Filter properties */
    channel->voltage_readout.filter.alpha = 10;
    channel->current_readout.filter.alpha = 10;

}

static t_psu_channel* psu_get_channel_from_node_id(t_psu_channel* channels, uint8_t count, uint8_t node_id)
{
    uint16_t i = 0;

    t_psu_channel* ret = NULL;

    for (i = 0; i < count; i++)
    {
        if (channels[i].node_id == node_id)
        {
            ret = &channels[i];
        }
    }

    return ret;
}

/**
 * Parse the received datagram(s)
 */
void remote_decode_datagram(t_psu_channel *channels, uint8_t count)
{
    bool new;
    bool crc_ok;
    uint8_t i = 0;
    t_psu_channel *temp_ch;
    uint8_t *packet_data = NULL;
    uint8_t packet_len = 0;
    uint8_t node_id;
    uint8_t identifier;

    do
    {
        new = remote_receive_get(&remote_dgram_rcv_copy);

        if (new == true)
        {
            crc_ok = remote_calc_crc_buffer_and_compare(remote_dgram_rcv_copy.data,
                                                        remote_dgram_rcv_copy.datagram.len,
                                                        remote_dgram_rcv_copy.datagram.crc,
                                                        NULL);

            if ((crc_ok == true) && (remote_dgram_rcv_copy.datagram.len > 1U))
            {

                /* node-id and packet type */
                node_id = remote_dgram_rcv_copy.data[0];
                identifier = remote_dgram_rcv_copy.data[1];

                /* Get the associated channel, if possible
                 * NOTE: channel id is divided by 2 (it is * 2 on slave!) */
                temp_ch = psu_get_channel_from_node_id(channels, count, node_id);

                /* data identifier: pointer to 2 bytes after the start */
                packet_data  = remote_dgram_rcv_copy.data;
                packet_data += 2;

                packet_len = remote_dgram_rcv_copy.datagram.len - 2;

                switch (identifier)
                {
                case DATAYPE_DEBUG:
                    /* debug data (usually a string) */
                    //uart_putstring(remote_dgram_rcv_copy.data);
                    //uart_putstring("\r\n");
                    break;
                case DATATYPE_CONFIG:

                    if (packet_len >= 16U)
                    {
                        /* config data */
                        temp_ch->voltage_readout.scale.max = lib_bytes_to_uint16(packet_data[0], packet_data[1]);
                        temp_ch->current_readout.scale.max = lib_bytes_to_uint16(packet_data[2], packet_data[3]);
                        temp_ch->voltage_readout.scale.max_scaled = lib_bytes_to_uint16(packet_data[4], packet_data[5]);
                        temp_ch->current_readout.scale.max_scaled = lib_bytes_to_uint16(packet_data[6], packet_data[7]);
                        temp_ch->voltage_setpoint.scale.max = lib_bytes_to_uint16(packet_data[8], packet_data[9]);
                        temp_ch->current_setpoint.scale.max = lib_bytes_to_uint16(packet_data[10], packet_data[11]);
                        temp_ch->voltage_setpoint.scale.max_scaled = lib_bytes_to_uint16(packet_data[12], packet_data[13]);
                        temp_ch->current_setpoint.scale.max_scaled = lib_bytes_to_uint16(packet_data[14], packet_data[15]);
                        /* Config received, set to operational */
                        temp_ch->state = PSU_STATE_OPERATIONAL;
                    }

                    break;
                case DATATYPE_READOUTS:
                    /* readout data */

                    if (packet_len >= 4u)
                    {
                        /* Voltage measurement for the remote channel */
                        temp_ch->voltage_readout.value.raw = lib_bytes_to_uint16(packet_data[0], packet_data[1]);
                        /* Current measurement for the remote channel */
                        temp_ch->current_readout.value.raw = lib_bytes_to_uint16(packet_data[2], packet_data[3]);
                        /* heartbeat notify */
                        temp_ch->heartbeat_ticks = 0U;
                    }

                    break;
                case DATATYPE_SETPOINTS:
                    /* setpoints data */

                    if (packet_len >= 4U)
                    {
                        /* Voltage setpoint for the remote channel */
                        temp_ch->voltage_setpoint.value.raw = lib_bytes_to_uint16(packet_data[0], packet_data[1]);
                        /* Current setpoint for the remote channel */
                        temp_ch->current_setpoint.value.raw = lib_bytes_to_uint16(packet_data[2], packet_data[3]);
                        /* heartbeat notify */
                        temp_ch->heartbeat_ticks = 0U;
                    }

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

void remote_encode_datagram(e_datatype type, t_psu_channel *channel)
{

    t_remote_datagram_buffer *rem_buf;
    remote_send_alloc(&rem_buf);
    uint8_t *data;

    if (rem_buf != NULL)
    {

        data = rem_buf->data;

        /* node-id */
        *(data++) = channel->node_id;

        switch (type)
        {
        case DATAYPE_DEBUG:
            /* debug data (usually a string) */
            break;
        case DATATYPE_CONFIG:
            /* config data */

            /* Datatype */
            *(data++) = DATATYPE_CONFIG;
            /* ADC voltage resolution */
            lib_uint16_to_bytes(channel->voltage_readout.scale.max, data, data + 1);
            data+=2;
            /* ADC current resolution */
            lib_uint16_to_bytes(channel->current_readout.scale.max, data, data + 1);
            data+=2;

            /* Voltage scaled maximum */
            lib_uint16_to_bytes(channel->voltage_readout.scale.max_scaled, data, data + 1);
            data+=2;
            /* Current scaled maximum */
            lib_uint16_to_bytes(channel->current_readout.scale.max_scaled, data, data + 1);
            data+=2;

            /* PWM voltage resolution */
            lib_uint16_to_bytes(channel->voltage_setpoint.scale.max, data, data + 1);
            data+=2;
            /* PWM current resolution */
            lib_uint16_to_bytes(channel->current_setpoint.scale.max, data, data + 1);
            data+=2;

            /* PWM Voltage scaled maximum */
            lib_uint16_to_bytes(channel->voltage_setpoint.scale.max_scaled, data, data + 1);
            data+=2;
            /* PWM Current scaled maximum */
            lib_uint16_to_bytes(channel->current_setpoint.scale.max_scaled, data, data + 1);
            data+=2;

            break;
        case DATATYPE_READOUTS:
            /* readout data */

            /* Datatype */
            *(data++) = DATATYPE_READOUTS;
            /* Voltage measurement for the remote channel */
            lib_uint16_to_bytes(channel->voltage_readout.value.raw, data, data + 1);
            data+=2;
            /* Current measurement for the remote channel */
            lib_uint16_to_bytes(channel->current_readout.value.raw, data, data + 1);
            data+=2;

            break;
        case DATATYPE_SETPOINTS:
            /* setpoints data */

            /* Datatype */
            *(data++) = DATATYPE_SETPOINTS;
            /* Voltage setpoint for the remote channel */
            lib_uint16_to_bytes(channel->voltage_setpoint.value.raw, data, data + 1);
            data+=2;
            /* Current setpoint for the remote channel */
            lib_uint16_to_bytes(channel->current_setpoint.value.raw, data, data + 1);
            data+=2;

            break;
        default:
            break;
        }

        /* set byte length */
        rem_buf->datagram.len = (uint8_t)(rem_buf->data - data);

        /* push the new datagram for next sending operation */
        remote_send_push();
    }
    else
    {
        /* Send buffer overflow */

    }
}

void psu_check_channel(t_psu_channel *psu_channel, bool tick)
{

    if ((tick == true) && (psu_channel->heartbeat_ticks < UINT8_MAX))
    {
        /* advance the tick counter */
        psu_channel->heartbeat_ticks++;
    }
    else
    {
        /* wait the tick timer */
    }

    switch(psu_channel->state)
    {
    case PSU_STATE_INIT:

        /* Send 3 copies of config frames */
        remote_encode_datagram(DATATYPE_CONFIG, psu_channel);

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
        /* Check for timeouts */

        if (psu_channel->heartbeat_ticks > PSU_TIMEOUT_TICKS)
        {
            psu_channel->state = PSU_STATE_SAFE_STATE;
        }
        else
        {
            /* Hardware is sane! */
        }

        break;
    case PSU_STATE_SAFE_STATE:
        break;
    default:
        break;
    }
}

void psu_set_measurement(t_measurement *measurement, t_value_type value)
{
    measurement->value.raw = value;
}

void psu_set_setpoint(t_psu_setpoint *measurement, t_value_type value)
{
    measurement->value.raw = value;
}

void psu_set_setpoint_scale(t_psu_setpoint *setpoint,
        t_value_type min, t_value_type max,
        t_value_type min_scaled, t_value_type max_scaled)
{
    setpoint->scale.min = min;
    setpoint->scale.max = max;
    setpoint->scale.min_scaled = min_scaled;
    setpoint->scale.max_scaled = max_scaled;
}

void psu_set_measurement_scale(t_measurement *measurement,
        t_value_type min, t_value_type max,
        t_value_type min_scaled, t_value_type max_scaled)
{
    measurement->scale.min = min;
    measurement->scale.max = max;
    measurement->scale.min_scaled = min_scaled;
    measurement->scale.max_scaled = max_scaled;
}
