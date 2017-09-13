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

#include "wiced_result.h"
#include "apollo_log.h"
#include "bluetooth_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

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
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/*
 * PLL tuning helper primitives (create/destroy/run/rest)
 */

wiced_result_t pll_tuner_create ( bt_audio_context_t *player );
wiced_result_t pll_tuner_destroy( bt_audio_context_t *player );
wiced_result_t pll_tuner_run    ( bt_audio_context_t *player );
wiced_result_t pll_tuner_rest   ( bt_audio_context_t *player );

#ifdef __cplusplus
} /* extern "C" */
#endif
