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

#include "platform.h" /* Include the platform to determine if it has buttons */

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

typedef enum
{
    BACK_BUTTON,
    FORWARD_BUTTON,
    VOLUME_UP_BUTTON,
    VOLUME_DOWN_BUTTON,
    PLAY_PAUSE_BUTTON,
    MULTI_FUNCTION_BUTTON,
} application_button_t;

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

#ifdef WICED_PLATFORM_BUTTON_COUNT

wiced_result_t app_init_button_interface();

#else
/* Default button initialization for platforms without buttons */
wiced_result_t app_init_button_interface()
{
    return WICED_SUCCESS;
}
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
