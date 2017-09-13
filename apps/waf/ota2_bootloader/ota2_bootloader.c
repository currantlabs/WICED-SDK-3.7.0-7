/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "wicedfs.h"
#include "platform_dct.h"
#include "elf.h"
#include "wiced_framework.h"
#include "wiced_utilities.h"
#include "platform_config.h"
#include "platform_resource.h"
#include "waf_platform.h"
#include "wwd_rtos.h"
#include "wiced_rtos.h"

#include "spi_flash.h"
#include "platform.h"
#include "platform_init.h"
#include "platform_stdio.h"
#include "platform_peripheral.h"
#include "platform_dct.h"

#include "wiced_ota2_image.h"

#if 1
#include "mini_printf.h"
#define BOOTLOADER_PRINTF(arg) {mini_printf arg;}
#else
#define BOOTLOADER_PRINTF(arg)
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#ifndef OTA2_BOOTLOADER_NO_SOFTAP_SUPPORT
#define SOFTAP_START_BUTTON_MILLISECONDS_MIN     4000           // allow 4 - 6 seconds
#define SOFTAP_START_BUTTON_MILLISECONDS_MAX     6000
#define FACTORY_RESET_BUTTON_MILLISECONDS_MIN    9000           // allow 9 - 15 seconds
#define FACTORY_RESET_BUTTON_MILLISECONDS_MAX   15000
#else
/* no softap on startup here */
#define FACTORY_RESET_BUTTON_MILLISECONDS_MIN    PLATFORM_FACTORY_RESET_TIMEOUT           // allow 5 - 10 seconds
#define FACTORY_RESET_BUTTON_MILLISECONDS_MAX    PLATFORM_FACTORY_RESET_TIMEOUT*2
#endif

/******************************************************
 *                   Enumerations
 ******************************************************/
typedef enum
{
    START_SEQUENCE_RUN_APP          = 0,
    START_SEQUENCE_FACTORY_RESET,
    START_SEQUENCE_OTA_UPDATE,
    START_SEQUENCE_SOFT_AP,

} bootloader_start_sequence_t;


typedef enum
{
    START_RESULT_OK = 0,
    START_RESULT_ERROR_BATTERY_LOW,
    START_RESULT_ERROR_DCT_COPY_FAIL,
    START_RESULT_ERROR_UPGRADE_FAIL,
    START_RESULT_ERROR_FACTORY_RESET_FAIL,

} bootloader_start_result_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/

static wiced_result_t load_program( const load_details_t * load_details, uint32_t* new_entry_point );

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

int main( void )
{
    bootloader_start_sequence_t start_sequence;
    const load_details_t*       load_details;
    uint32_t                    entry_point;
    boot_detail_t               boot;
    wiced_result_t              result;
    bootloader_start_result_t   boot_result;

    wiced_ota2_image_status_t   factory_reset_app_status;
    wiced_ota2_image_status_t   staging_image_status;

    uint32_t                    button_pressed_milliseconds;
    platform_dct_ota2_config_t  dct_ota2_config;

    boot_result = START_RESULT_OK;

    BOOTLOADER_PRINTF(("OTA2 Bootloader!\r\n"));

    /* Assume a normal boot */
    start_sequence = START_SEQUENCE_RUN_APP;
    wiced_waf_app_set_boot( DCT_APP0_INDEX, PLATFORM_DEFAULT_LOAD );

    /* set up timing for no Operating System, we haven't started an OS yet */
    NoOS_setup_timing( );

    /* Determine if OTA Image Update is available
     *
     * NOTE: This must NOT over-ride the factory reset OR SoftAP sequences
     *       So we check it first, but do not act on it until later.
     *
    */
    result = wiced_ota2_image_get_status ( WICED_OTA2_IMAGE_TYPE_STAGED, &staging_image_status );
    if ((result == WICED_SUCCESS) && (staging_image_status == WICED_OTA2_IMAGE_EXTRACT_ON_NEXT_BOOT))
    {
        start_sequence = START_SEQUENCE_OTA_UPDATE;
    }

    /* check for factory reset button being pressed */
    button_pressed_milliseconds = wiced_waf_get_button_press_time();
    if (button_pressed_milliseconds > 1000)
    {
        BOOTLOADER_PRINTF(("button_pressed_milliseconds: %d\r\n", button_pressed_milliseconds));
    }

    NoOS_stop_timing( );

#ifndef OTA2_BOOTLOADER_NO_SOFTAP_SUPPORT
    /* determine if button held for ~5 seconds, start SoftAP for manual OTA upload */
    if ((button_pressed_milliseconds >= SOFTAP_START_BUTTON_MILLISECONDS_MIN) &&
        (button_pressed_milliseconds <= SOFTAP_START_BUTTON_MILLISECONDS_MAX))
    {
        /* go into SoftAP, DHCP & minimal WebServer */
        start_sequence = START_SEQUENCE_SOFT_AP;
    }
#endif

    /* determine if button held for ~10 seconds, run factory reset */
    wiced_dct_read_with_copy( &dct_ota2_config, DCT_OTA2_CONFIG_SECTION, 0, sizeof(platform_dct_ota2_config_t) );
    if ( ((button_pressed_milliseconds >= FACTORY_RESET_BUTTON_MILLISECONDS_MIN) &&
          (button_pressed_milliseconds <= FACTORY_RESET_BUTTON_MILLISECONDS_MAX)) ||
         (dct_ota2_config.force_factory_reset != 0) )
    {
        /* OTA Image push button factory reset here !!! */
        start_sequence = START_SEQUENCE_FACTORY_RESET;
        result = wiced_ota2_image_get_status( WICED_OTA2_IMAGE_TYPE_FACTORY_RESET_APP, &factory_reset_app_status );
        if ((result != WICED_SUCCESS) && (factory_reset_app_status != WICED_OTA2_IMAGE_VALID))
        {
            BOOTLOADER_PRINTF(("Factory Reset Image invalid !!!!!!!!!!!!\r\n"));
            start_sequence = START_SEQUENCE_RUN_APP;
        }

    }

#ifndef WICED_DISABLE_WATCHDOG
    if (platform_watchdog_check_last_reset() == WICED_TRUE)
    {
        start_sequence = START_SEQUENCE_FACTORY_RESET;
        BOOTLOADER_PRINTF(("platform_watchdog_check_last_reset() reported a Watchdog Reset - force factory reset!!\r\n"));
    }
#if 0
    /* enable to test watchdog
     *  - after downloading the code you will see
     *      OTA2 Bootloader!
     *      wiced_ota2_get_status(1) Download valid.
     *      wiced_ota2_get_status() Current App - OK for now, TODO: check CRC when added to apps_lut!
     *      wiced_ota2_get_status(4) Download complete. Extract on Reboot.
     *  - then ~10 seconds later
     *  OTA2 Bootloader!
     *  wiced_ota2_get_status(1) Download valid.
     *  wiced_ota2_get_status() Current App - OK for now, TODO: check CRC when added to apps_lut!
     *  wiced_ota2_get_status(4) Download complete. Extract on Reboot.
     *  platform_watchdog_check_last_reset() reported a Watchdog Reset - force factory reset!!
     *
     */
    if (start_sequence != START_SEQUENCE_FACTORY_RESET)
    {
        while(1);
    }
#endif
#endif

#ifdef CHECK_BATTERY_LEVEL_BEFORE_OTA2_UPGRADE
    if ((start_sequence == START_SEQUENCE_OTA_UPDATE) ||
        (start_sequence == START_SEQUENCE_FACTORY_RESET))
    {
        /* check for battery level before doing any writing! */
        if (platform_check_battery_level(CHECK_BATTERY_LEVEL_OTA2_UPGRADE_MINIMUM) != WICED_SUCCESS)
        {
            /* check_battery_level() failed */
            battery_ok = 0;
            start_sequence = START_SEQUENCE_RUN_APP;
            boot_result = START_RESULT_ERROR_BATTERY_LOW;
        }
    }
#endif /* CHECK_BATTERY_LEVEL_BEFORE_OTA2_UPGRADE */


    result = WICED_SUCCESS;
    if ((start_sequence == START_SEQUENCE_OTA_UPDATE) ||
        (start_sequence == START_SEQUENCE_SOFT_AP) ||
        (start_sequence == START_SEQUENCE_FACTORY_RESET))
    {
        /* set a flag of the boot type for the extraction, and see if we want to start ota2_extract app (OTA_APP)
         */
        switch (start_sequence)
        {
            default:
                break;
            case START_SEQUENCE_FACTORY_RESET:
                start_sequence = START_SEQUENCE_RUN_APP;
                dct_ota2_config.boot_type = OTA2_BOOT_FACTORY_RESET;
                wiced_dct_write( &dct_ota2_config, DCT_OTA2_CONFIG_SECTION, 0, sizeof(platform_dct_ota2_config_t) );
                wiced_waf_app_set_boot( DCT_OTA_APP_INDEX, PLATFORM_DEFAULT_LOAD );
                break;
            case START_SEQUENCE_OTA_UPDATE:
                start_sequence = START_SEQUENCE_RUN_APP;
                dct_ota2_config.boot_type = OTA2_BOOT_UPDATE;
                wiced_dct_write( &dct_ota2_config, DCT_OTA2_CONFIG_SECTION, 0, sizeof(platform_dct_ota2_config_t) );
                wiced_waf_app_set_boot( DCT_OTA_APP_INDEX, PLATFORM_DEFAULT_LOAD );
                break;
#ifndef OTA2_BOOTLOADER_NO_SOFTAP_SUPPORT
            case START_SEQUENCE_SOFT_AP:
                start_sequence = START_SEQUENCE_RUN_APP;
                dct_ota2_config.boot_type = OTA2_SOFTAP_UPDATE;
                wiced_dct_write( &dct_ota2_config, DCT_OTA2_CONFIG_SECTION, 0, sizeof(platform_dct_ota2_config_t) );
                wiced_waf_app_set_boot( DCT_OTA_APP_INDEX, PLATFORM_DEFAULT_LOAD );
                break;
#endif
        }
    }

    BOOTLOADER_PRINTF(("OTA2 Bootloader -- start_sequence : %d ", start_sequence));

    /* boot the device with the current app */
    boot.load_details.valid = 1;
    boot.entry_point        = 0;
    wiced_dct_read_with_copy( &boot, DCT_INTERNAL_SECTION, OFFSET( platform_dct_header_t, boot_detail ), sizeof(boot_detail_t) );

    load_details = &boot.load_details;
    entry_point  = boot.entry_point;

    if ( load_details->valid != 0 )
    {
        if (load_program( load_details, &entry_point ) == WICED_SUCCESS)
        {
            BOOTLOADER_PRINTF(("wiced_waf_start_app() 0x%lx\r\n", entry_point  ));
        }
    }

    wiced_waf_start_app( entry_point );

    while(1)
    {
        (void)boot_result;
    }

    /* Should never get here */
    return 0;
}

static wiced_result_t load_program( const load_details_t * load_details, uint32_t* new_entry_point )
{
    wiced_result_t result = WICED_ERROR;

    if ( load_details->destination.id == EXTERNAL_FIXED_LOCATION )
    {
        /* External serial flash destination. Currently not allowed */
        result = WICED_ERROR;
    }
    else if ( load_details->source.id == EXTERNAL_FIXED_LOCATION )
    {
        /* Fixed location in serial flash source - i.e. no filesystem */
        result = wiced_waf_app_load( &load_details->source, new_entry_point );
    }
    else if ( load_details->source.id == EXTERNAL_FILESYSTEM_FILE )
    {
        /* Filesystem location in serial flash source */
        result = wiced_waf_app_load( &load_details->source, new_entry_point );
    }

    if ( load_details->load_once != 0 )
    {
        boot_detail_t boot;

        boot.entry_point                 = 0;
        boot.load_details.load_once      = 1;
        boot.load_details.valid          = 0;
        boot.load_details.destination.id = INTERNAL;
        wiced_dct_write_boot_details( &boot );
    }

    return result;
}
