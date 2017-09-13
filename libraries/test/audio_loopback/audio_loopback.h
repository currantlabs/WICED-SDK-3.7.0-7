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

#include "wiced.h"

/******************************************************
 *                     Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/* Control whether the library logs to console */
#ifndef WICED_AUDIO_LOOPBACK_LOG_ENABLED
#define WICED_AUDIO_LOOPBACK_LOG_ENABLED                (1)
#endif

/* Transmit a sine wave instead of data from the SDIN line. */
#ifndef WICED_AUDIO_LOOPBACK_ENABLE_SINE_WAVE_OUTPUT
#define WICED_AUDIO_LOOPBACK_ENABLE_SINE_WAVE_OUTPUT    (0)
#endif

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
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t wiced_audio_loopback_run( uint32_t iterations );
wiced_result_t wiced_audio_loopback_start( void );
wiced_result_t wiced_audio_loopback_stop( void );

#ifdef __cplusplus
} /*extern "C" */
#endif



