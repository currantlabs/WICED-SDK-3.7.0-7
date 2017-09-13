/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/*
 * Defines internal configuration of the BCM94909WCD1_3 board
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  OTA Support
 *  NOTE:
 *  1. Only apply when OTA2 is not enabled.
 *  2. Following line must be located ahead of platform_config_bsp_default.h to override PLATFORM_NO_SFLASH_WRITE.
 */
#ifndef OTA2_SUPPORT
#define PLATFORM_HAS_OTA
#endif

/*
 * Below configuration line makes possible to have code and non-BSS data in DDR.
 */
#ifdef PLATFORM_NO_DDR
#define PLATFORM_DDR_CODE_AND_DATA_ENABLE !PLATFORM_NO_DDR
#else
#define PLATFORM_DDR_CODE_AND_DATA_ENABLE 1
#endif

/*
 * Below configuration file defines default BSP settings.
 * Put here platform specific configuration parameters to override these default ones.
 */
#include "platform_config_bsp_default.h"

/*
 * Below configuration file defines default WICED settings.
 * To change settings replace below included file with its contents.
 */
#include "platform_config_wiced_default.h"

#ifdef __cplusplus
} /* extern "C" */
#endif

