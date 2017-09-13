/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *
 */

#include "wiced_framework.h"

#include "ota2_test_dct.h"
#include "wifi_config_dct.h"

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
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

DEFINE_APP_DCT(ota2_dct_t)
{
    .reboot_count       = 0,

    .ota2_update_ap_info =
    {
        .details = {{sizeof(OTA2_UPDATE_SSID)-1, OTA2_UPDATE_SSID},
                    {{0,0,0,0,0,0}}, 0, 0, OTA2_UPDATE_BSS_TYPE, OTA2_UPDATE_SECURITY, OTA2_UPDATE_CHANNEL, OTA2_UPDATE_BAND},
        .security_key_length = sizeof(OTA2_UPDATE_PASSPHRASE)-1,
        .security_key = OTA2_UPDATE_PASSPHRASE,
    },
};

/******************************************************
 *               Function Definitions
 ******************************************************/
