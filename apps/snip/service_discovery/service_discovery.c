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
 * Service Discovery Application
 *
 * Features demonstrated
 *  - Gedday service discovery library
 *
 * Application Instructions
 *  1. Modify the CLIENT_AP_SSID/CLIENT_AP_PASSPHRASE Wi-Fi credentials
 *     in wifi_config_dct.h header file to match your Wi-Fi access point.
 *  2. Modify MDNS_SERVICE_TOBE_DISCOVERED macro based on the mDNS service to be discovered.
 *  3. Build and download the application as described in WICED Quick Start Guide.
 *  4. Connects to the AP and discover the mDNS service available on the network.
 */

#include "wiced.h"
#include "gedday.h"

/******************************************************
 *                      Macros
 ******************************************************/
#define MDNS_SERVICE_TOBE_DISCOVERED        "_wiced123._tcp.local"

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

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{
    wiced_result_t  res;
    gedday_service_t service_result;
    int i = 0;
    char service_name[] = MDNS_SERVICE_TOBE_DISCOVERED;

    wiced_init( );

    res = wiced_network_up(WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL);
    if (res != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("Failed to bring network up. Error [%d]\n", res));
        return;
    }

    res = gedday_init(WICED_STA_INTERFACE, "WICED_Gedday_Discovery_Example");
    if (res != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("Failed to init Gedday. Error [%d]\n", res));
        return;
    }

    WPRINT_APP_INFO(("Discovering service [%s]\n", MDNS_SERVICE_TOBE_DISCOVERED));
    while ( 1 )
    {
        WPRINT_APP_INFO(("Discover Try [%d]\n", i++));
        memset(&service_result, 0x0, sizeof(service_result));
        res = gedday_discover_service(service_name, &service_result);
        if (res != WICED_SUCCESS)
        {
            WPRINT_APP_INFO(("Service Discovery Failed with Error [%d]\n", res));
        }
        else
        {
            WPRINT_APP_INFO(("Service Discovery Result:\n"));
            WPRINT_APP_INFO(("  Service Name  = [%s]\n", service_result.service_name));
            WPRINT_APP_INFO(("  Instance Name = [%s]\n", service_result.instance_name));
            WPRINT_APP_INFO(("  Host Name     = [%s]\n", service_result.hostname));
            WPRINT_APP_INFO(("  TXT Record    = [%s]\n", service_result.txt));
            WPRINT_APP_INFO(("  Port          = [%d]\n", service_result.port));

            /* Free the records */
            if (service_result.instance_name != NULL)
            {
                free(service_result.instance_name);
            }
            if (service_result.hostname != NULL)
            {
                free(service_result.hostname);
            }
            if (service_result.txt != NULL)
            {
                free(service_result.txt);
            }
        }

        wiced_rtos_delay_milliseconds( 500 );
    }
}
