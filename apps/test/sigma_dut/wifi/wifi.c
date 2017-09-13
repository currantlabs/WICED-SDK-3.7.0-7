/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include "wiced_wifi.h"
#include "string.h"
#include "wwd_debug.h"
#include "command_console.h"
#include "wwd_assert.h"
#include "wwd_network.h"
#include "stdlib.h"
#include "wwd_management.h"
#include "WWD/internal/wwd_sdpcm.h"
#include "wifi.h"
#include "internal/wwd_internal.h"
#include "network/wwd_buffer_interface.h"
#include "wiced_management.h"
#include "dhcp_server.h"
#include "wiced_crypto.h"
#include "wiced.h"


#define MAX_SSID_LEN 32
#define MAX_PASSPHRASE_LEN 64



int wifi_join(char* ssid, uint8_t ssid_length, wiced_security_t auth_type, uint8_t* key, uint16_t key_length, char* ip, char* netmask, char* gateway)
{
    wiced_network_config_t network_config;
    wiced_ip_setting_t* ip_settings = NULL;
    wiced_ip_setting_t static_ip_settings;
    platform_dct_wifi_config_t* dct_wifi_config;

    if (wwd_wifi_is_ready_to_transceive(WWD_STA_INTERFACE) == WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }

    // Read config
    wiced_dct_read_lock( (void**) &dct_wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
//    print_wifi_config_dct();

    // Modify config
    memcpy((char*)dct_wifi_config->stored_ap_list[0].details.SSID.value, ssid, ssid_length);
    dct_wifi_config->stored_ap_list[0].details.SSID.length = ssid_length;
    dct_wifi_config->stored_ap_list[0].details.security = auth_type;
    memcpy((char*)dct_wifi_config->stored_ap_list[0].security_key, (char*)key, MAX_PASSPHRASE_LEN);
    dct_wifi_config->stored_ap_list[0].security_key_length = key_length;

    // Write config
    wiced_dct_write( (const void*) dct_wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
   //    print_wifi_config_dct();

    /* Tell the network stack to setup it's interface */
    if ( ip == NULL )
    {
        network_config = WICED_USE_EXTERNAL_DHCP_SERVER;
    }
    else
    {
        network_config = WICED_USE_STATIC_IP;
        if ( str_to_ip( ip, &static_ip_settings.ip_address ) != WICED_SUCCESS )
        {
            static_ip_settings.ip_address.version = WICED_IPV4; /* Force to IPv4 in case the IP address was terminated with a space rather than a NULL */
        }
        if ( str_to_ip( netmask, &static_ip_settings.netmask ) != WICED_SUCCESS )
        {
            static_ip_settings.netmask.version = WICED_IPV4; /* Force to IPv4 in case the netmask was terminated with a space rather than a NULL */
        }
        if ( str_to_ip( gateway, &static_ip_settings.gateway ) != WICED_SUCCESS )
        {
            static_ip_settings.gateway.version = WICED_IPV4; /* Force to IPv4 in case the gateway address was terminated with a space rather than a NULL */
        }
        ip_settings = &static_ip_settings;
    }

    if ( wiced_network_up( WICED_STA_INTERFACE, network_config, ip_settings ) != WICED_SUCCESS )
    {
        return ERR_UNKNOWN;
    }

    return ERR_CMD_OK;
}


//static wiced_result_t print_wifi_config_dct( void )
//{
//    platform_dct_wifi_config_t const* dct_wifi_config = wiced_dct_get_wifi_config_section( );
//
//    WPRINT_APP_INFO( ( "\n----------------------------------------------------------------\n\n") );
//
//    /* Wi-Fi Config Section */
//    WPRINT_APP_INFO( ( "Wi-Fi Config Section \n") );
//    WPRINT_APP_INFO( ( "    device_configured               : %d \n", dct_wifi_config->device_configured ) );
//    WPRINT_APP_INFO( ( "    stored_ap_list[0]  (SSID)       : %s \n", dct_wifi_config->stored_ap_list[0].details.SSID.val ) );
//    WPRINT_APP_INFO( ( "    stored_ap_list[0]  (Passphrase) : %s \n", dct_wifi_config->stored_ap_list[0].security_key ) );
//    WPRINT_APP_INFO( ( "    soft_ap_settings   (SSID)       : %s \n", dct_wifi_config->soft_ap_settings.SSID.val ) );
//    WPRINT_APP_INFO( ( "    soft_ap_settings   (Passphrase) : %s \n", dct_wifi_config->soft_ap_settings.security_key ) );
//    WPRINT_APP_INFO( ( "    config_ap_settings (SSID)       : %s \n", dct_wifi_config->config_ap_settings.SSID.val ) );
//    WPRINT_APP_INFO( ( "    config_ap_settings (Passphrase) : %s \n", dct_wifi_config->config_ap_settings.security_key ) );
//    WPRINT_APP_INFO( ( "    country_code                    : %c%c%d \n", ((dct_wifi_config->country_code) >>  0) & 0xff,
//                                                                            ((dct_wifi_config->country_code) >>  8) & 0xff,
//                                                                            ((dct_wifi_config->country_code) >> 16) & 0xff));
//    //print_mac_address( "    DCT mac_address                 :", (wiced_mac_t*)&dct_wifi_config->mac_address );
//
//    return WICED_SUCCESS;
//}
