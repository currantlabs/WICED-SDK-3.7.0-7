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

#include <stdarg.h>
#include "wiced_result.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                     Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    AUDIO_PLAYER_LOG_OFF = 0,
    AUDIO_PLAYER_LOG_ERR,
    AUDIO_PLAYER_LOG_WARNING,
    AUDIO_PLAYER_LOG_NOTICE,
    AUDIO_PLAYER_LOG_INFO,
    AUDIO_PLAYER_LOG_DEBUG0,
    AUDIO_PLAYER_LOG_DEBUG1,
    AUDIO_PLAYER_LOG_DEBUG2,
    AUDIO_PLAYER_LOG_DEBUG3,
    AUDIO_PLAYER_LOG_DEBUG4,

    AUDIO_PLAYER_LOG_MAX
} AUDIO_PLAYER_LOG_LEVEL_T;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t  audio_player_log_init(AUDIO_PLAYER_LOG_LEVEL_T level);
wiced_result_t  audio_player_log_shutdown(void);
wiced_result_t  audio_player_log_set_level(AUDIO_PLAYER_LOG_LEVEL_T level);
AUDIO_PLAYER_LOG_LEVEL_T audio_player_log_get_level(void);
wiced_result_t  audio_player_log_msg(AUDIO_PLAYER_LOG_LEVEL_T level, const char *fmt, ...);
wiced_result_t  audio_player_printf(const char *fmt, ...);

#ifdef __cplusplus
} /* extern "C" */
#endif
