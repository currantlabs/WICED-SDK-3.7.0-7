/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#pragma once

#include <stdint.h>
#include "compat.h"
#include "wwd_constants.h"
#include "wiced_rtos.h"
#include "wiced_network.h"
#include "wiced_time.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_STREAM_TABLE_ENTRIES ( 4 )

typedef enum
{
    TRAFFIC_ANY        = 0x00,
    TRAFFIC_SEND       = 0x01,
    TRAFFIC_RECV       = 0x02,
} traffic_direction_t;

typedef enum
{
    PROFILE_FILE_TRANSFER = 0x01,
    PROFILE_MULTICAST     = 0x02,
    PROFILE_IPTV          = 0x03,
    PROFILE_TRANSACTION   = 0x04,
} traffic_profile_t;

typedef struct
{
    uint8_t allocated; // Indicates if stream table entry has been allocated for use
    uint8_t enabled; // Indicates if stream is currently active
    uint32_t stream_id;
    traffic_profile_t profile;
    traffic_direction_t direction;
    char dest_ipaddr[16];
    uint16_t dest_port;
    char src_ipaddr[16];
    uint16_t src_port;
    int payload_size;
    int frame_rate; // Frames per second
    int duration;
    int start_delay;
    wiced_qos_access_category_t ac;
    int max_frame_count;
    int frames_sent;
    int frames_received;
    int out_of_sequence_frames;
    int bytes_sent;
    int bytes_received;
    wiced_udp_socket_t tx_socket;
    wiced_udp_socket_t rx_socket;
    wiced_time_t stop_time;
    void* thread_ptr; // This allows the console thread to delete threads
} traffic_stream_t;

typedef struct
{
    wiced_thread_t thread_handle;
    traffic_stream_t *ts;
    int (*ts_function)( traffic_stream_t * );
} thread_details_t;

extern wiced_mutex_t  tx_done_mutex;

int udp_rx( traffic_stream_t* ts );
int udp_tx( traffic_stream_t* ts );
int udp_transactional( traffic_stream_t* ts );


#ifdef __cplusplus
} /* extern "C" */
#endif
