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

#include "audio_player_types.h"
#include "platform_audio.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define AUDIO_PLAYER_BUFFERING_MS_MIN        0
#define AUDIO_PLAYER_BUFFERING_MS_DEFAULT   50
#define AUDIO_PLAYER_BUFFERING_MS_MAX     1000

#define AUDIO_PLAYER_THRESHOLD_MS_MIN        0
#define AUDIO_PLAYER_THRESHOLD_MS_DEFAULT   40
#define AUDIO_PLAYER_THRESHOLD_MS_MAX     1000

#define AUDIO_PLAYER_VOLUME_MIN              0
#define AUDIO_PLAYER_VOLUME_DEFAULT         70
#define AUDIO_PLAYER_VOLUME_MAX            100

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

typedef struct audio_player_app_dct_s
{
    AUDIO_CHANNEL_MAP_T channel;
    int buffering_ms;
    int threshold_ms;
    int clock_enable;   /* enable AS clock */
    int volume;
    platform_audio_device_id_t audio_device_rx;
    platform_audio_device_id_t audio_device_tx;
} audio_player_app_dct_t;


#ifdef __cplusplus
} /* extern "C" */
#endif
