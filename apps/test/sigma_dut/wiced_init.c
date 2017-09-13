/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "command_console.h"
#include "wiced_management.h"
#include "wifi_cert/wifi_cert_commands.h"

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
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

#define MAX_LINE_LENGTH  (256)
#define MAX_HISTORY_LENGTH (20)

#ifdef WICED_USE_WIFI_P2P_INTERFACE
extern wiced_result_t my_set_data_rate( uint32_t rate, wwd_interface_t interface );
#endif

static char line_buffer[MAX_LINE_LENGTH];
static char history_buffer_storage[MAX_LINE_LENGTH * MAX_HISTORY_LENGTH];

static const command_t commands[] = {
    WIFI_CERT_COMMANDS
    WIFI_P2P_CERT_COMMANDS
    CMD_TABLE_END
};


wiced_mutex_t  tx_done_mutex;

/**
 *  @param thread_input : Unused parameter - required to match thread prototype
 */
void application_start( void )
{
    /* Initialise the device */
    wiced_init( );

    printf( "Sigma_dut app\n" );

    wiced_rtos_init_mutex( &tx_done_mutex );

    if ( create_ping_worker_thread( ) != WICED_SUCCESS )
    {
        printf( "Error initialising ping worker thread\n" );
    }

#ifdef WICED_USE_WIFI_P2P_INTERFACE
    my_set_data_rate( 6, WWD_STA_INTERFACE ); // This is just for P2P so that our pings get through
#endif

    /* Run the main application function */
    command_console_init( STDIO_UART, MAX_LINE_LENGTH, line_buffer, MAX_HISTORY_LENGTH, history_buffer_storage, "," );
    console_add_cmd_table( commands );
}

