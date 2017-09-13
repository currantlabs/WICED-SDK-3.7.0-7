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

#ifdef __cplusplus
extern "C" {
#endif

#include "wiced_platform.h"
#include "platform_audio.h"
#include "apollocore.h"
#include "apollo_log.h"

#ifndef APOLLO_NO_BT
#include "apollo_bt_nv.h"
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define APOLLO_BUFFERING_MS_MIN             0
#define APOLLO_BUFFERING_MS_DEFAULT         50
#define APOLLO_BUFFERING_MS_MAX             1000

#define APOLLO_THRESHOLD_MS_MIN             0
#define APOLLO_THRESHOLD_MS_DEFAULT         40
#define APOLLO_THRESHOLD_MS_MAX             1000

#define APOLLO_VOLUME_MIN                   0
#define APOLLO_VOLUME_DEFAULT               70
#define APOLLO_VOLUME_MAX                   100

/* BURST LENGTH: for SLC use auto, for non SLC use 6 */
#define APOLLO_BURST_LENGTH_DEFAULT         APOLLO_STREAMER_BURST_AUTO_SLC

#define APOLLO_AUDIO_DEVICE_STRING_LENGTH   16

#define APOLLO_INPUT_SAMPLE_RATE_DEFAULT    PLATFORM_AUDIO_SAMPLE_RATE_48KHZ
#define APOLLO_INPUT_SAMPLE_SIZE_DEFAULT    PLATFORM_AUDIO_SAMPLE_SIZE_16_BIT
#define APOLLO_INPUT_CHANNEL_COUNT          2

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

#ifdef APOLLO_NO_BT
typedef struct
{
    int placeholder;
} apollo_bt_dummy_t;

typedef struct
{
    apollo_bt_dummy_t bt_hash_table;
    apollo_bt_dummy_t bt_paired_device_info[1];
    apollo_bt_dummy_t bt_local_id_keys[1];
} apollo_bt_dct_t;
#endif

typedef struct
{
    int                             is_configured;
    apollo_role_t                   apollo_role;
    char                            speaker_name[APOLLO_SPEAKER_NAME_LENGTH];
    APOLLO_CHANNEL_MAP_T            speaker_channel;
    APOLLO_LOG_LEVEL_T              log_level;
    int                             buffering_ms;
    int                             threshold_ms;
    int                             auto_start;
    int                             clock_enable;   /* enable AS clock */
    int                             pll_tuning_enable;
    int                             volume;
    int                             payload_size;
    int                             burst_length;
    int                             shuffle_length;
    int                             source_type;
    wiced_ip_address_t              clientaddr;     /* Client IP address when sending audio */
    int                             rtp_port;

    platform_audio_device_id_t      audio_device_rx;     /* Audio capture device  */
    platform_audio_device_id_t      audio_device_tx;     /* Audio playback device */
    platform_audio_sample_rates_t   input_sample_rate;
    platform_audio_sample_sizes_t   input_sample_size;
    uint8_t                         input_channel_count;
} apollo_dct_t;

typedef struct
{
    apollo_dct_t    apollo_dct;
    apollo_bt_dct_t apollo_bt_dct;
} app_dct_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
