/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file Audio Player Utility Routines
 *
 */

#include "wiced.h"
#include "audio_player_util.h"

/******************************************************
 *                      Macros
 ******************************************************/

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

/******************************************************
 *               Static Function Declarations
 ******************************************************/

static wiced_bool_t audio_player_log_initialized;
static AUDIO_PLAYER_LOG_LEVEL_T audio_player_log_level;

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t audio_player_log_init(AUDIO_PLAYER_LOG_LEVEL_T level)
{
    if (audio_player_log_initialized != WICED_FALSE)
    {
        return WICED_ERROR;
    }

    audio_player_log_initialized  = WICED_TRUE;
    audio_player_log_level        = level;

    return WICED_SUCCESS;
}

wiced_result_t audio_player_log_shutdown(void)
{
    if (audio_player_log_initialized == WICED_FALSE)
    {
        return WICED_ERROR;
    }

    audio_player_log_initialized = WICED_FALSE;

    return WICED_SUCCESS;
}

wiced_result_t audio_player_log_set_level(AUDIO_PLAYER_LOG_LEVEL_T level)
{
    if (audio_player_log_initialized == WICED_FALSE)
    {
        return WICED_ERROR;
    }

    if (level >= AUDIO_PLAYER_LOG_OFF && level < AUDIO_PLAYER_LOG_MAX)
    {
        audio_player_log_level = level;
    }

    return WICED_SUCCESS;
}

AUDIO_PLAYER_LOG_LEVEL_T audio_player_log_get_level(void)
{
    if (audio_player_log_initialized == WICED_FALSE)
    {
        return AUDIO_PLAYER_LOG_OFF;
    }

    return audio_player_log_level;
}

wiced_result_t audio_player_log_msg(AUDIO_PLAYER_LOG_LEVEL_T level, const char *fmt, ...)
{
    va_list args;

    if (audio_player_log_initialized == WICED_FALSE)
    {
        return WICED_ERROR;
    }

    if (audio_player_log_level == AUDIO_PLAYER_LOG_OFF || level > audio_player_log_level)
    {
        return WICED_SUCCESS;
    }

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    return WICED_SUCCESS;
}

wiced_result_t audio_player_printf(const char *fmt, ...)
{
    va_list args;

    if (audio_player_log_initialized == WICED_FALSE)
    {
        return WICED_ERROR;
    }

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    return WICED_SUCCESS;
}
