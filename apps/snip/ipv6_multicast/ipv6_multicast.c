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
 * MLD IPV6 multicast protocol demonstration
 *
 *   1. This app connects to a Wi-Fi network as a STA (client).
 *   2. Application then joins 2 widely used IPV6 multicast groups -
 *      MDNS - ff02::fb
 *      SSDP - ff02::0c
 *   3. In wireshark you should be able to see at least two "MLD Listener report" frames after the join is complete.
 *   4. 10 seconds later the application will call Network stack to leave the above multicast groups.
 *   5. In wireshark you should be able to see at least two valid "MLD Listener Done" frames.
 *
 *   6. Current MLD implementation also supports responding to the "MLD Listener Query" frames which may
 *      be sent periodically from the querier( router ) to solicit reports of all multicast addresses of interest on
 *      the network.
 *
 */

#include "wiced.h"

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
 *               Variable Definitions
 ******************************************************/
wiced_ip_address_t s_address_multicast_v6;

/******************************************************
 *               Function Definitions
 ******************************************************/
void application_start()
{

    wiced_init();
    wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );

    /* Join an IPV6 MDNS multicast group */
    s_address_multicast_v6.version = WICED_IPV6;
    SET_IPV6_ADDRESS( s_address_multicast_v6, MAKE_IPV6_ADDRESS( 0xff02, 0, 0, 0, 0, 0, 0, 0xfb) );
    wiced_multicast_join( WICED_STA_INTERFACE , &s_address_multicast_v6 );
    WPRINT_APP_INFO(("MDNS IPV6 multicast group has been joined\r\n"));

    /* Join an IPV6 SSDP multicast group */
    s_address_multicast_v6.version = WICED_IPV6;
    SET_IPV6_ADDRESS( s_address_multicast_v6, MAKE_IPV6_ADDRESS( 0xff02, 0, 0, 0, 0, 0, 0, 0x0c) );
    wiced_multicast_join( WICED_STA_INTERFACE , &s_address_multicast_v6 );
    WPRINT_APP_INFO(("SSDP IPV6 multicast group has been joined\r\n"));

    /* Keep attached to the above multicast groups for about 10 seconds */
    wiced_rtos_delay_milliseconds(10000);


    /* Leave IPV6 MDNS multicast group */
    s_address_multicast_v6.version = WICED_IPV6;
    SET_IPV6_ADDRESS( s_address_multicast_v6, MAKE_IPV6_ADDRESS( 0xff02, 0, 0, 0, 0, 0, 0, 0xfb) );
    wiced_multicast_leave( WICED_STA_INTERFACE , &s_address_multicast_v6 );
    WPRINT_APP_INFO(("MDNS IPV6 multicast group has been left\r\n"));

    /* Leave IPV6 SSDP multicast group */
    s_address_multicast_v6.version = WICED_IPV6;
    SET_IPV6_ADDRESS( s_address_multicast_v6, MAKE_IPV6_ADDRESS( 0xff02, 0, 0, 0, 0, 0, 0, 0x0c) );
    wiced_multicast_leave( WICED_STA_INTERFACE , &s_address_multicast_v6 );
    WPRINT_APP_INFO(("SSDP IPV6 multicast group has been left\r\n"));

}
