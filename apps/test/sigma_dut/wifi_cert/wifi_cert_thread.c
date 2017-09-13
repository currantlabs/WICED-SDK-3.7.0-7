/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "command_console.h"
#include "wifi_cert_traffic_stream.h"
#include "wiced_defaults.h"
#include "wiced_tcpip.h"
#include "wwd_wlioctl.h"

#define TS_THREAD_STACK_SIZE ( 4096 )

extern traffic_stream_t stream_table[NUM_STREAM_TABLE_ENTRIES];
extern int ac_priority[AC_COUNT];

void ts_thread_wrapper( uint32_t arg );

int spawn_ts_thread( void *ts_function, traffic_stream_t *ts )
{
    thread_details_t* detail = NULL;

    detail = calloc( 1, sizeof( thread_details_t ) );

    if ( detail == NULL )
    {
        printf( "calloc fail when spawning traffic stream thread\n" );
        return ERR_UNKNOWN;
    }
    detail->ts_function = ts_function;
    detail->ts = ts;
    detail->ts->thread_ptr = detail;

    int prio = WICED_APPLICATION_PRIORITY + 1; // Default + 1 priority for TRAFFIC_ANY direction

//#if ( defined(NETWORK_LwIP) )
    if ( ts->direction == TRAFFIC_SEND )
    {
        prio = WICED_APPLICATION_PRIORITY + 1 + ac_priority[ts->ac];
    }
    else if ( ts->direction == TRAFFIC_RECV )
    {
       ;//prio = 1;
    }
//#endif

    if ( WICED_SUCCESS !=  wiced_rtos_create_thread( &detail->thread_handle, prio, (const char*)"ts_thread", (wiced_thread_function_t)ts_thread_wrapper, TS_THREAD_STACK_SIZE, (void *)detail ))
    {
        printf("failed to create thread\n");
        return ERR_UNKNOWN;
    }

    wiced_rtos_delay_milliseconds(10);

    return ERR_CMD_OK;
}


void ts_thread_wrapper( uint32_t arg )
{
    thread_details_t* detail = (thread_details_t*) arg;
    //wiced_thread_t *tmp_hnd = NULL;

    //printf( "Started thread 0x%08X\n", (unsigned int)detail );


    if ( ( detail->ts->direction == TRAFFIC_RECV ) || ( detail->ts->direction == TRAFFIC_ANY ))
    {
        printf("status,COMPLETE\n" );
    }

    detail->ts_function( detail->ts );

    // All threads will exit to here. We use a mutex to ensure only one tx thread prints results for all tx threads that ran concurrently
    //printf( "Thread 0x%08X exited with return value %d\n", (unsigned int)detail, result );
    host_rtos_delay_milliseconds( 1000 ); // Delay to ensure the linux endpoint is ready and all local transmit streams are finished
    traffic_stream_t *ts = detail->ts;

    if ( ts->direction == TRAFFIC_SEND )
    {
        wiced_rtos_lock_mutex( &tx_done_mutex );
    }

    if ( ts->direction == TRAFFIC_SEND && ts->enabled ) // Print results from all tx streams terminating at the same time
    {
        int i;

        // Disable all transmit streams that finish at the same time so we only print one set of results
        for ( i = 0; i < NUM_STREAM_TABLE_ENTRIES; ++i )
        {
            if ( ts->stop_time == stream_table[i].stop_time && stream_table[i].direction == TRAFFIC_SEND )
            {
                stream_table[i].enabled = 0;
            }
        }

        wiced_rtos_delay_milliseconds( 1000 ); // Delay to ensure the linux endpoint is ready and all local transmit streams are finished

        // Print results for all transmit streams that finished at the same time
        printf("status,COMPLETE,streamID," );

        for ( i = 0; i < NUM_STREAM_TABLE_ENTRIES; ++i )
        {
            if ( ts->stop_time == stream_table[i].stop_time && stream_table[i].direction == TRAFFIC_SEND )
            {
                printf( "%u ", (unsigned int)stream_table[i].stream_id );
            }
        }

        printf( ",txFrames," );
        for ( i = 0; i < NUM_STREAM_TABLE_ENTRIES; ++i )
        {
            if ( ts->stop_time == stream_table[i].stop_time && stream_table[i].direction == TRAFFIC_SEND )
            {
                if (stream_table[i].frames_sent == 0)
                {
                    stream_table[i].frames_sent = 1; // If it's not set to a positive number the Sigma script locks up
                }
                printf( "%d ", stream_table[i].frames_sent );
            }
        }

        printf( ",rxFrames," );
        for ( i = 0; i < NUM_STREAM_TABLE_ENTRIES; ++i )
        {
            if ( ts->stop_time == stream_table[i].stop_time && stream_table[i].direction == TRAFFIC_SEND )
            {
                printf( "%d ", stream_table[i].frames_received );
            }
        }

        printf( ",txPayloadBytes," );
        for ( i = 0; i < NUM_STREAM_TABLE_ENTRIES; ++i )
        {
            if ( ts->stop_time == stream_table[i].stop_time && stream_table[i].direction == TRAFFIC_SEND )
            {
                printf( "%d ", stream_table[i].bytes_sent );
            }
        }

        printf( ",rxPayloadBytes," );
        for ( i = 0; i < NUM_STREAM_TABLE_ENTRIES; ++i )
        {
            if ( ts->stop_time == stream_table[i].stop_time && stream_table[i].direction == TRAFFIC_SEND )
            {
                printf( "%d ", stream_table[i].bytes_received );
            }
        }

        printf( ",outOfSequenceFrames," );
        for ( i = 0; i < NUM_STREAM_TABLE_ENTRIES; ++i )
        {
            if ( ts->stop_time == stream_table[i].stop_time && stream_table[i].direction == TRAFFIC_SEND )
            {
                printf( "%d ", stream_table[i].out_of_sequence_frames );
            }
        }

        printf( "\n" );
        printf( "> " );

    }

    if ( ts->direction == TRAFFIC_SEND )
    {
        wiced_rtos_unlock_mutex( &tx_done_mutex );

        // Free the socket so we can reuse the port
        wiced_udp_delete_socket( &ts->tx_socket );
    }

    WICED_END_OF_THREAD(NULL);
}
