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

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#define APP_CHANGED_FLAG        0x64DEAD46

/* This is the default AP the device will connect to (as a client)*/

#define OTA2_UPDATE_SSID       "YOUR_OTA2_AP"           /* AP connected to upgrade server */
#define OTA2_UPDATE_PASSPHRASE "YOUR_OTA2_AP_PASSWORD"
#define OTA2_UPDATE_BSS_TYPE   WICED_BSS_TYPE_INFRASTRUCTURE
#define OTA2_UPDATE_SECURITY   WICED_SECURITY_WPA2_AES_PSK
#define OTA2_UPDATE_CHANNEL    44
#define OTA2_UPDATE_BAND       WICED_802_11_BAND_5GHZ


/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct ota2_dct_s
{
        uint32_t                    reboot_count;
        wiced_config_ap_entry_t     ota2_update_ap_info;      /* AP used for getting OTA updates */
} ota2_dct_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
