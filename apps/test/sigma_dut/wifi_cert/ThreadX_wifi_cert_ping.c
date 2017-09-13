/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "compat.h"
#include "tx_thread.h"
#include "wiced_management.h"
#include "wiced_wifi.h"
#include "wwd_debug.h"
#include "wwd_assert.h"
#include "network/wwd_buffer_interface.h"
#include "RTOS/wwd_rtos_interface.h"
#include "wwd_network_constants.h"
#include "wwd_network.h"
#include "wiced_network.h"
#include "command_console.h"
#include "wiced_p2p.h"
#include "wiced_time.h"

static uint32_t      num_ping_requests = 0;
static uint32_t      num_ping_replies = 0;


wiced_result_t wifi_traffic_send_ping( void *arg )
{
    wiced_ip_address_t ping_target;
    NX_PACKET *response_ptr;
    char ping_payload[1500];
    wiced_time_t send_time;
    wiced_time_t recv_time;
    wiced_time_t finish_time;
    uint32_t interface = 0;
    UINT status;
    char** argv = arg;

    int frame_size = atoi(argv[4]);
    int frame_rate = atoi(argv[6]); // Frames per second

    // XXX Some scripts send a decimal frame rate even though the variable is defined as a short int. This fix may not work for all test plans.
    if ( frame_rate == 0 )
    {
        frame_rate = 1;
    }

    int duration = atoi(argv[8]);  // How long to ping for in seconds
    int frame_interval = 1000 / frame_rate;
    int num_frames = (frame_rate * duration);
    uint32_t wait_time = 0;

    num_ping_requests = 0; // Global count of the number of ping requests for the latest call of this function
    num_ping_replies = 0; // Global count of the number of ping replies for the latest call of this function
    if ( str_to_ip( argv[2], &ping_target ) != WICED_SUCCESS )
    {
        ping_target.version = WICED_IPV4; /* Force to IPv4 in case the IP address was terminated with a space rather than a NULL */
    }

    // This is limited to one interface being up
    if (wwd_wifi_is_ready_to_transceive(WICED_STA_INTERFACE) == WWD_SUCCESS)
    {
        interface = WICED_STA_INTERFACE;
    }
    else if (wwd_wifi_is_ready_to_transceive(WICED_AP_INTERFACE) == WWD_SUCCESS)
    {
        interface = WICED_AP_INTERFACE;
    }
    else if (wwd_wifi_is_ready_to_transceive(WICED_P2P_INTERFACE) == WWD_SUCCESS)
    {
        interface = WICED_P2P_INTERFACE;
    }
    else
    {
        return 0;
    }

    if ( ( num_frames < 5 ) && besl_p2p_group_owner_is_up() == WICED_TRUE )
    {
        wait_time = 15000; // To cope with STAs in power save
    }
    else
    {
        wait_time = (uint32_t)(frame_interval - 5);
    }


    // Clear the NetX ARP pipe by sending a sacrificial ping of the required size first and then waiting 100 ms
    status = nx_icmp_ping( &IP_HANDLE(interface), ping_target.ip.v4, ping_payload, frame_size, &response_ptr, 100 );
    if ( status == NX_SUCCESS )
    {
        nx_packet_release( response_ptr );
    }
    host_rtos_delay_milliseconds( 100 );

    finish_time = host_rtos_get_time( ) + ( duration * 1000 );

    while ( (num_frames > 0 ) && ( host_rtos_get_time( ) < finish_time ) )
    {
        ++num_ping_requests;
        send_time = host_rtos_get_time( );
        status = nx_icmp_ping( &IP_HANDLE(interface), ping_target.ip.v4, ping_payload, frame_size, &response_ptr, wait_time * SYSTICK_FREQUENCY / 1000 );
        recv_time = host_rtos_get_time( );

        /* Print result */
        if ( status == NX_SUCCESS )
        {
            ++num_ping_replies;
            nx_packet_release( response_ptr );
        }
        else
        {
            ;//printf(" ping not ok 0x%x\n", (unsigned int) status);
        }

        /* Sleep until time for next ping */
        if ( ( recv_time - send_time ) < frame_interval )
        {
            host_rtos_delay_milliseconds( frame_interval - ( recv_time - send_time ) );
        }

        --num_frames; // Decrement frames to be sent
    }

    host_rtos_delay_milliseconds( 10 );

    return 0;
}

/*!
 ******************************************************************************
 * Return value of num_ping_requests and num_ping_replies
 * @return  0 for success, otherwise error
 */

int wifi_traffic_stop_ping( void )
{
    printf("status,COMPLETE,sent,%u,replies,%u\n", (unsigned int)num_ping_requests,(unsigned int)num_ping_replies);

    return ERR_CMD_OK;
}

