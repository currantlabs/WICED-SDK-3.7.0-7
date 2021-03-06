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
 * OTA Image test Application
 *
 * Over
 * The
 * Air
 *
 * Update Test
 *
 * This application snippet demonstrates how to use the WICED
 * interface for performing Over The Air Updates to your device
 *
 * Application Instructions
 * 1. wifi_config_dct.h: Modify the CLIENT_AP_SSID/CLIENT_AP_PASSPHRASE Wi-Fi credentials
 *    in the header file to match your Wi-Fi access point
 * 2. ota2_test_dct.h: Modify the OTA2_UPDATE_SSID/OTA2_UPDATE_PASSPHRASE Wi-Fi credentials
 *    in the header file to match your Wi-Fi access point for connecting to your OTA2 image file server
 *    NOTE: If both APs are the same, use NULL in ota2_test_get_update() in this file:
 *
 *    player->ota2_bg_params.ota2_ap_info = NULL;
 *
 * 3. Connect a PC terminal to the serial port of the WICED Eval board,
 *    then build and download the application as described in the WICED
 *    Quick Start Guide
 *
 * After the download completes, it connects to the Wi-Fi AP specified in apps/snip/ota2_test/wifi_config_dct.h
 *
 * When you issue a "get_update <host>/ota2_image_file" the app will connect directly ONCE to download the OTA2 Image file
 *                  <ota2_image_file> from <host>.
 *
 * When you issue a "timed_update" <host>/ota2_image_file" the app will connect to download the OTA2 Image file
 *                  <ota2_image_file> from <host> on a regularly timed basis
 *                  See ota2_test_get_update() in this file.
 *
 *  case OTA2_EXAMPLE_UPDATE_TYPE_TIMED:
 *      player->ota2_bg_params.initial_check_interval   = 5;            <-- start in 5 seconds
 *      player->ota2_bg_params.check_interval           = 5 * 60;       <-- regularly timed check every 5 minutes
 *      player->ota2_bg_params.retry_check_interval     = 5;            <-- on Failure, retry in 5 seconds
 *
 */

#include "ctype.h"
#include "wiced.h"
#include "wiced_tcpip.h"
#include "platform.h"
#include "command_console.h"
#include "command_console_wifi.h"
#include "console_wl.h"
#include "resources.h"
#include "internal/wwd_sdpcm.h"
#include "wiced_dct_common.h"

#include "ota2_test.h"
#include "ota2_test_config.h"

#ifdef WWD_TEST_NVRAM_OVERRIDE
#include "internal/bus_protocols/wwd_bus_protocol_interface.h"
#endif

#include "wiced_ota2_image.h"
#include "wiced_ota2_service.h"
#include "wiced_ota2_network.h"
#include "../../WICED/internal/wiced_internal_api.h"

#include "../../utilities/mini_printf/mini_printf.h"

/******************************************************
 *                      Macros
 ******************************************************/
#define CHECK_IOCTL_BUFFER( buff )  if ( buff == NULL ) {  wiced_assert("Allocation failed\n", 0 == 1); return WWD_BUFFER_ALLOC_FAIL; }
#define CHECK_RETURN( expr )  { wwd_result_t check_res = (expr); if ( check_res != WWD_SUCCESS ) { wiced_assert("Command failed\n", 0 == 1); return check_res; } }

#define Mod32_GT( A, B )        ( (int32_t)( ( (uint32_t)( A ) ) - ( (uint32_t)( B ) ) ) >   0 )

#define OTA2_CONSOLE_COMMANDS \
    { (char*) "exit",           ota2_console_command,    0, NULL, NULL, (char *)"", (char *)"Exit application" }, \
    { (char*) "log",            ota2_console_command,    0, NULL, NULL, (char *)"", (char *)"Set log level (0-5)" }, \
    { (char*) "get_update",     ota2_console_command,    0, NULL, NULL, (char *)"", (char *)"Get OTA2 update - use connection" }, \
    { (char*) "timed_update",   ota2_console_command,    0, NULL, NULL, (char *)"", (char *)"Get Timed OTA2 update" }, \
    { (char*) "stop_update",    ota2_console_command,    0, NULL, NULL, (char *)"", (char *)"Stop Timed OTA2 update" }, \
    { (char*) "factory_status", ota2_console_command,    0, NULL, NULL, (char *)"", (char *)"Factory Reset - show status" }, \
    { (char*) "factory_reboot", ota2_console_command,    0, NULL, NULL, (char *)"", (char *)"Factory Reset on Reboot" }, \
    { (char*) "factory_now",    ota2_console_command,    0, NULL, NULL, (char *)"", (char *)"Factory Reset - extract NOW" }, \
    { (char*) "update_now",     ota2_console_command,    0, NULL, NULL, (char *)"", (char *)"OTA2 update - Update from staging now" }, \
    { (char*) "update_reboot",  ota2_console_command,    0, NULL, NULL, (char *)"", (char *)"OTA2 update - Update from staging on boot" }, \
    { (char*) "update_status",  ota2_console_command,    0, NULL, NULL, (char *)"", (char *)"OTA2 update - show status / valid" }, \
    { (char*) "config",         ota2_console_command,    0, NULL, NULL, (char *)"", (char *)"Display / change config values" }, \
    { (char*) "disconnect",     ota2_console_command,    0, NULL, NULL, (char *)"", (char *)"Disconnect from AP" }, \
    { (char*) "default_ap",     ota2_console_command,    0, NULL, NULL, (char *)"", (char *)"Join Default AP" }, \
    { (char*) "ota2_ap",        ota2_console_command,    0, NULL, NULL, (char *)"", (char *)"Join OTA2 AP" }, \
    { (char*) "status",         ota2_console_command,    0, NULL, NULL, (char *)"", (char *)"show status" }, \
    { (char*) "wlog",           read_wlan_chip_console_log,         0, NULL, NULL, (char*) "",                                           (char*) "Dump WLAN chip console log"}, \


/******************************************************
 *                    Constants
 ******************************************************/

#define MY_DEVICE_NAME                      "ota2_test"
#define MY_DEVICE_MODEL                     "1.0"
#define MAX_COMMAND_LENGTH                   (85)
#define CONSOLE_COMMAND_HISTORY_LENGTH      (10)

#define FIRMWARE_VERSION                    "wiced-1.0"

#define OTA2_UPDATE_FILE_NAME          "/brcmtest/OTA2_image_file.ota_image"

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum {
    OTA2_CONSOLE_CMD_EXIT = 0,

    OTA2_CONSOLE_CMD_CONFIG,

    OTA2_CONSOLE_CMD_DISCONNECT_AP,
    OTA2_CONSOLE_CMD_CONNECT_DEFAULT_AP,
    OTA2_CONSOLE_CMD_CONNECT_OTA2_AP,
    OTA2_CONSOLE_CMD_STATUS,

    OTA2_CONSOLE_CMD_GET_UPDATE,
    OTA2_CONSOLE_CMD_GET_TIMED_UPDATE,
    OTA2_CONSOLE_CMD_STOP_TIMED_UPDATE,

    OTA2_CONSOLE_CMD_FACTORY_RESET_STATUS,
    OTA2_CONSOLE_CMD_FACTORY_RESET_REBOOT,
    OTA2_CONSOLE_CMD_FACTORY_NOW,

    OTA2_CONSOLE_CMD_UPDATE_NOW,
    OTA2_CONSOLE_CMD_UPDATE_REBOOT,
    OTA2_CONSOLE_CMD_UPDATE_STATUS,

    OTA2_CONSOLE_CMD_LOG_LEVEL,


    OTA2_CONSOLE_CMD_MAX,
} OTA2_CONSOLE_CMDS_T;

#define NUM_NSECONDS_IN_SECOND                      (1000000000LL)
#define NUM_USECONDS_IN_SECOND                      (1000000)
#define NUM_NSECONDS_IN_MSECOND                     (1000000)
#define NUM_NSECONDS_IN_USECOND                     (1000)
#define NUM_USECONDS_IN_MSECOND                     (1000)
#define NUM_MSECONDS_IN_SECOND                      (1000)

typedef enum {
    OTA2_EXAMPLE_UPDATE_TYPE_NOW    =   0,
    OTA2_EXAMPLE_UPDATE_TYPE_TIMED,

} ota2_example_update_type_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/


/******************************************************
 *                    Structures
 ******************************************************/

typedef struct cmd_lookup_s {
        char *cmd;
        uint32_t event;
} cmd_lookup_t;

/******************************************************
 *               Function Declarations
 ******************************************************/

int ota2_console_command(int argc, char *argv[]);

/******************************************************
 *               Variables Definitions
 ******************************************************/
#define WICED_OTA2_BUFFER_NODE_COUNT         (256)
#define RESTORE_DCT_APP_SETTINGS             (1)

#ifdef PLATFORM_L1_CACHE_BYTES
#define NUM_BUFFERS_POOL_SIZE(x)       ((WICED_LINK_MTU_ALIGNED + sizeof(wiced_packet_t) + 1) * (x))
#define APP_RX_BUFFER_POOL_SIZE        NUM_BUFFERS_POOL_SIZE(WICED_OTA2_BUFFER_NODE_COUNT)
#endif

#ifdef PLATFORM_L1_CACHE_BYTES
uint8_t                          ota2_rx_packets[APP_RX_BUFFER_POOL_SIZE + PLATFORM_L1_CACHE_BYTES]        __attribute__ ((section (".external_ram")));
#else
uint8_t                          ota2_rx_packets[WICED_NETWORK_MTU_SIZE * WICED_OTA2_BUFFER_NODE_COUNT]     __attribute__ ((section (".external_ram")));
#endif

static char ota2_command_buffer[MAX_COMMAND_LENGTH];
static char ota2_command_history_buffer[MAX_COMMAND_LENGTH * CONSOLE_COMMAND_HISTORY_LENGTH];

uint8_t ota2_thread_stack_buffer[OTA2_THREAD_STACK_SIZE]                               __attribute__ ((section (".bss.ccm")));

const command_t ota2_command_table[] = {
    OTA2_CONSOLE_COMMANDS
    WL_COMMANDS
    CMD_TABLE_END
};

static cmd_lookup_t command_lookup[OTA2_CONSOLE_CMD_MAX] = {
        { "exit",           PLAYER_EVENT_SHUTDOWN             },
        { "config",         0                                 },
        { "disconnect",     PLAYER_EVENT_DISCONNECT_AP        },
        { "default_ap",     PLAYER_EVENT_CONNECT_DEFAULT_AP   },
        { "ota2_ap",        PLAYER_EVENT_CONNECT_OTA2_AP      },
        { "status",         PLAYER_EVENT_STATUS               },
        { "get_update",     PLAYER_EVENT_GET_UPDATE           },
        { "timed_update",   PLAYER_EVENT_GET_TIMED_UPDATE     },
        { "stop_update",    PLAYER_EVENT_STOP_TIMED_UPDATE    },
        { "factory_status", PLAYER_EVENT_FACTORY_RESET_STATUS },
        { "factory_reboot", PLAYER_EVENT_FACTORY_RESET_REBOOT },
        { "factory_now",    PLAYER_EVENT_FACTORY_RESET_NOW    },
        { "update_now",     PLAYER_EVENT_UPDATE_NOW           },
        { "update_reboot",  PLAYER_EVENT_UPDATE_REBOOT        },
        { "update_status",  PLAYER_EVENT_UPDATE_STATUS        },
        { "log",            PLAYER_EVENT_LOG_LEVEL            },
};

/* for when we are connecting to an ADHOC network */
static const wiced_ip_setting_t ap_ip_settings =
{
    INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
    INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS( 255,255,255,  0 ) ),
    INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
};

/* template for HTTP GET */
char ota2_get_request_template[] =
{
    "GET %s HTTP/1.1\r\n"
    "Host: %s%s \r\n"
    "\r\n"
};

const char* firmware_version = FIRMWARE_VERSION;

ota2_data_t *g_player;

/******************************************************
 *               Function Declarations
 ******************************************************/
wiced_result_t over_the_air_2_app_restore_settings_after_update( ota2_data_t* player );

static ota2_data_t* init_player(void);
static void ota2_test_mainloop(ota2_data_t *player);
static void ota2_test_shutdown(ota2_data_t *player)
;

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start(void)
{
    ota2_data_t*   player;

    /*
     * Main initialization.
     */

    if ((player = init_player()) == NULL)
    {
        return;
    }
    g_player = player;

    wiced_time_get_time( &g_player->start_time );

    /*
     * Drop into our main loop.
     */
    ota2_test_mainloop(player);

    /*
     * Cleanup and exit.
     */

    g_player = NULL;
    ota2_test_shutdown(player);
    player = NULL;
}

/****************************************************************
 *  Console command Function Declarations
 ****************************************************************/
int printf_memory( char *message, uint8_t*addr, uint16_t length)
{
    uint16_t offset, i = 0;
    if (addr == NULL)
        return 0;

    printf("\r\nMemory  addr:%p len:%d dump: %s\r\n", addr, length, message);
    for (offset = 0; offset < length; offset += 16)
    {
        printf("%p  ", &addr[offset]);
        for (i= offset; (i < (offset + 16)) && (i < length); i++)
        {
            printf("%02x ", addr[i]);
        }
        printf("    ");
        for (i= offset; (i < (offset + 16)) && (i < length); i++)
        {
            printf("%c ", (isprint(addr[i]) ? addr[i] : ' '));
        }
        printf("\r\n");
    }

    return offset + i;
}



int ota2_console_command(int argc, char *argv[])
{
    uint32_t event = 0;
    int i;

    OTA2_APP_PRINT(OTA2_LOG_INFO, ("Received command: %s\n", argv[0]));

    if (g_player == NULL || g_player->tag != PLAYER_TAG_VALID)
    {
        OTA2_APP_PRINT(OTA2_LOG_DEBUG, ("ota2_console_command() Bad player structure\r\n"));
        return ERR_CMD_OK;
    }

    /*
     * Lookup the command in our table.
     */

    for (i = 0; i < OTA2_CONSOLE_CMD_MAX; ++i)
    {
        if (strcmp(command_lookup[i].cmd, argv[0]) == 0)
            break;
    }

    if (i >= OTA2_CONSOLE_CMD_MAX)
    {
        OTA2_APP_PRINT(OTA2_LOG_DEBUG, ("Unrecognized command: %s\n", argv[0]));
        return ERR_CMD_OK;
    }

    switch (i)
    {
        case OTA2_CONSOLE_CMD_EXIT:
            break;
        case OTA2_CONSOLE_CMD_DISCONNECT_AP:
        case OTA2_CONSOLE_CMD_CONNECT_DEFAULT_AP:
        case OTA2_CONSOLE_CMD_CONNECT_OTA2_AP:
        case OTA2_CONSOLE_CMD_STATUS:
            event = command_lookup[i].event;
            break;
        case OTA2_CONSOLE_CMD_GET_UPDATE:
            memset(g_player->uri_to_stream, 0, sizeof(g_player->uri_to_stream));
            if (argc > 1)
            {
                strlcpy(g_player->uri_to_stream, argv[1], (sizeof(g_player->uri_to_stream) - 1) );
            }
            else
            {
                strlcpy(g_player->uri_to_stream, OTA2_UPDATE_FILE_NAME, (sizeof(g_player->uri_to_stream) - 1) );
            }
            OTA2_APP_PRINT(OTA2_LOG_INFO, (" Get update file:%s\r\n", g_player->uri_to_stream));
            event = command_lookup[i].event;
            break;
        case OTA2_CONSOLE_CMD_GET_TIMED_UPDATE:
            memset(g_player->uri_to_stream, 0, sizeof(g_player->uri_to_stream));
            if (argc > 1)
            {
                strlcpy(g_player->uri_to_stream, argv[1], sizeof(g_player->uri_to_stream) - 1 );
            }
            else
            {
                strlcpy(g_player->uri_to_stream, OTA2_UPDATE_FILE_NAME, (sizeof(g_player->uri_to_stream) - 1) );
            }
            OTA2_APP_PRINT(OTA2_LOG_INFO, (" Get update using BG service uri:%s \r\n", g_player->uri_to_stream));
            event = command_lookup[i].event;
            break;
        case OTA2_CONSOLE_CMD_LOG_LEVEL:
            if (argc > 1)
            {
                int new_level;
                new_level = atoi(argv[1]);
                if ((new_level > 0) && (new_level < OTA2_LOG_DEBUG1))
                {
                    g_player->log_level = new_level;
                    if (g_player->ota2_bg_service != NULL)
                    {
                        wiced_ota2_service_set_debug_log_level(g_player->ota2_bg_service, new_level);
                    }
                }
            }
            OTA2_APP_PRINT(OTA2_LOG_ALWAYS, ("log level = %d\r\n", g_player->log_level));
            break;
        case OTA2_CONSOLE_CMD_FACTORY_RESET_STATUS:
        case OTA2_CONSOLE_CMD_FACTORY_RESET_REBOOT:
        case OTA2_CONSOLE_CMD_FACTORY_NOW:
        case OTA2_CONSOLE_CMD_UPDATE_NOW:
        case OTA2_CONSOLE_CMD_UPDATE_REBOOT:
        case OTA2_CONSOLE_CMD_UPDATE_STATUS:
        case OTA2_CONSOLE_CMD_STOP_TIMED_UPDATE:
            event = command_lookup[i].event;
            break;

        case OTA2_CONSOLE_CMD_CONFIG:
            ota2_set_config(g_player, argc, argv);
            break;
    }

    if (event)
    {
        /*
         * Send off the event to the main loop.
         */

        wiced_rtos_set_event_flags(&g_player->events, event);
    }

    return ERR_CMD_OK;
}

wiced_result_t my_ota2_callback(void* session_id, wiced_ota2_service_status_t status, uint32_t value, void* opaque )
{
    ota2_data_t*    player = (ota2_data_t*)opaque;

    UNUSED_PARAMETER(session_id);
    UNUSED_PARAMETER(player);


    switch( status )
    {
    case OTA2_SERVICE_STARTED:      /* Background service has started
                                         * return - None                                                             */
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("----------------------------- OTA2 Service Called : SERVE STARTED -----------------------------\r\n"));
        break;
    case OTA2_SERVICE_AP_CONNECT_ERROR:
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("----------------------------- OTA2 Service Called : AP CONNECT_ERROR -----------------------------\r\n"));
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("        return SUCESS (not used by service). This is informational \r\n"));
        break;

    case OTA2_SERVICE_SERVER_CONNECT_ERROR:
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("----------------------------- OTA2 Service Called : SERVER_CONNECT_ERROR -----------------------------\r\n"));
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("        return SUCESS (not used by service). This is informational \r\n"));
        break;

    case OTA2_SERVICE_AP_CONNECTED:
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("----------------------------- OTA2 Service Called : AP_CONNECTED -----------------------------\r\n"));
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("        return SUCESS (not used by service). This is informational \r\n"));
        break;

    case OTA2_SERVICE_SERVER_CONNECTED:
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("----------------------------- OTA2 Service Called : SERVER_CONNECTED -----------------------------\r\n"));
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("        return SUCESS (not used by service). This is informational \r\n"));
        break;


    case OTA2_SERVICE_CHECK_FOR_UPDATE: /* Time to check for updates.
                                         * return - WICED_SUCCESS = Service will check for update availability
                                         *        - WICED_ERROR   = Application will check for update availability   */
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("----------------------------- OTA2 Service Called : CHECK_FOR_UPDATE -----------------------------\r\n"));
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("        return SUCCESS, let Service do the checking.\r\n"));
        return WICED_SUCCESS;

    case OTA2_SERVICE_UPDATE_AVAILABLE: /* Service has contacted server, update is available
                                         * return - WICED_SUCCESS = Application indicating that it wants the
                                         *                           OTA Service to perform the download
                                         *        - WICED_ERROR   = Application indicating that it will perform
                                         *                           the download, the OTA Service will do nothing.  */
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("----------------------------- OTA2 Service Called : UPDATE_AVAILABLE -----------------------------\r\n"));
        /* the OTA2 header for the update is pointed to by the value argument and is only valid for this function call */

        /*
         * In an actual application, the application would look at the headers information and decide if the
         * file on the update server is a newer version that the currently running application.
         *
         * If the application wants the update to continue, it would return WICED_SUCCESS here
         * If not, return WICED_ERROR
         *
         */
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("        return SUCCESS, let Service perform the download.\r\n"));
        return WICED_SUCCESS;

    case OTA2_SERVICE_DOWNLOAD_STATUS:  /* Download status - value has % complete (0-100)
                                         *   NOTE: This will only occur when Service is performing download
                                         * return - WICED_SUCCESS = Service will continue download
                                         *        - WICED_ERROR   = Service will STOP download and service will
                                         *                          issue OTA2_SERVICE_TIME_TO_UPDATE_ERROR           */
        OTA2_APP_PRINT(OTA2_LOG_DEBUG, ("my_ota2_callback() OTA2_SERVICE_DOWNLOAD_STATUS %ld %%!\r\n", value));
        return WICED_SUCCESS;

    case OTA2_SERVICE_PERFORM_UPDATE:   /* Download is complete
                                         * return - WICED_SUCCESS = Service will inform Bootloader to extract
                                         *                          and update on next power cycle
                                         *        - WICED_ERROR   = Service will inform Bootloader that download
                                         *                          is complete - Bootloader will NOT extract        */
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("----------------------------- OTA2 Service Called : PERFORM_UPDATE -----------------------------\r\n"));
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("        return SUCCESS, let Service extract update on next reboot.\r\n"));
        return WICED_SUCCESS;

    case OTA2_SERVICE_UPDATE_ERROR:     /* There was an error in transmission
                                         * This will only occur if Error during Service performing data transfer
                                         * return - WICED_SUCCESS = Service will retry immediately
                                         *        - WICED_ERROR   = Service will retry on next check_interval
                                         *            Application can call
                                         *            wiced_ota2_service_check_for_updates()
                                         *            to run another check earlier than next timed update            */
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("----------------------------- OTA2 Service Called : UPDATE_ERROR -----------------------------\r\n"));
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("        return SUCCESS, let Service use retry check interval.\r\n"));
        return WICED_SUCCESS;

    case OTA2_SERVICE_UPDATE_ENDED:     /* All update actions for this check are complete.
                                         * This callback is to allow the application to take any actions when
                                         * the service is done checking / downloading an update
                                         * (succesful, unavailable, or error)
                                         * return - None                                                             */
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("----------------------------- OTA2 Service Called : UPDATE_ENDED -----------------------------\r\n"));
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("        return SUCESS (not used by service). This is informational \r\n"));
        break;

    case OTA2_SERVICE_STOPPED:
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("----------------------------- OTA2 Service Called : SERVICE ENDED -----------------------------\r\n"));
        OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("        return SUCESS (not used by service). This is informational \r\n"));
        break;

    default:
        OTA2_APP_PRINT(OTA2_LOG_INFO, ("my_ota2_callback() UNKNOWN STATUS %d!\r\n", status));
        break;
    }

    return WICED_SUCCESS;
}

wiced_result_t ota2_test_get_update(ota2_data_t* player, ota2_example_update_type_t type)
{
    wiced_result_t result = WICED_ERROR;

    /* get the image from the server & save in staging area */

    wiced_ota2_service_uri_split(player->uri_to_stream, player->ota2_host_name, sizeof(player->ota2_host_name),
                                     player->ota2_file_path, sizeof(player->ota2_file_path), &player->ota2_bg_params.port);

    player->ota2_bg_params.host_name                = player->ota2_host_name;
    player->ota2_bg_params.file_path                = player->ota2_file_path;
    player->ota2_bg_params.auto_update              = 0;
    player->ota2_bg_params.initial_check_interval   = 5;            /* initial check in 5 seconds */
    player->ota2_bg_params.check_interval           = 10 * 60;      /* 10 minutes - use SECONDS_IN_24_HOURS for 1 day */
    player->ota2_bg_params.retry_check_interval     = 5;            /* in some fail cases, retry after 5 seconds */
    player->ota2_bg_params.ota2_ap_info             = &player->dct_app->ota2_update_ap_info;    /* pass NULL if the APs are the same */
    player->ota2_bg_params.default_ap_info          = &player->dct_wifi->stored_ap_list[0];

    if (player->ota2_bg_service != NULL)
    {
        switch (type)
        {
        case OTA2_EXAMPLE_UPDATE_TYPE_NOW:
            OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("OTA2 session already inited, just use previous session for GET NOW!\r\n"));
            break;
        case OTA2_EXAMPLE_UPDATE_TYPE_TIMED:
            OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("OTA2 session already inited, deinit() previous session!\r\n"));
            wiced_ota2_service_deinit(player->ota2_bg_service);
            player->ota2_bg_service = NULL;
            break;
        }

    }

    OTA2_APP_PRINT(OTA2_LOG_ERROR, ("ota2_test_get_update() player->ota2_bg_service %p \r\n", player->ota2_bg_service));
    if (player->ota2_bg_service == NULL)
    {
        player->ota2_bg_service = wiced_ota2_service_init(&player->ota2_bg_params, player);
        OTA2_APP_PRINT(OTA2_LOG_ERROR, ("ota2_test_get_update() wiced_ota2_service_init() bg_service:%p \r\n", player->ota2_bg_service));
    }
    if (player->ota2_bg_service != NULL)
    {
        /* add a callback */
        result = wiced_ota2_service_register_callback(player->ota2_bg_service, my_ota2_callback);
        if (result != WICED_SUCCESS)
        {
                OTA2_APP_PRINT(OTA2_LOG_ERROR, ("ota2_test_get_update register callback failed! %d \r\n", result));
                wiced_ota2_service_deinit(player->ota2_bg_service);
                player->ota2_bg_service = NULL;
        }

        if (player->ota2_bg_service != NULL)
        {
            switch (type)
            {
            case OTA2_EXAMPLE_UPDATE_TYPE_NOW:
                OTA2_APP_PRINT(OTA2_LOG_ALWAYS, ("Download the OTA Image file - get it NOW!\r\n"));
                /* NOTE: THis is a blocking call! */
                result = wiced_ota2_service_check_for_updates(player->ota2_bg_service);
                if (result != WICED_SUCCESS)
                {
                        OTA2_APP_PRINT(OTA2_LOG_ERROR, ("ota2_test_get_update wiced_ota2_service_check_for_updates() failed! %d \r\n", result));
                }
                break;

            case OTA2_EXAMPLE_UPDATE_TYPE_TIMED:
                OTA2_APP_PRINT(OTA2_LOG_ALWAYS, ("Download the OTA Image file - get it Timed !\r\n"));
                /* NOTE: This is a non-blocking call (async) */
                result = wiced_ota2_service_start(player->ota2_bg_service);
                if (result != WICED_SUCCESS)
                {
                        OTA2_APP_PRINT(OTA2_LOG_ERROR, ("ota2_test_get_update wiced_ota2_service_start() failed! %d \r\n", result));
                }
                /* do not de-init the service - it needs to be running for background update to work ! */
                break;
            }
        }
    }
    return result;
}

wiced_result_t ota2_test_stop_timed_update(ota2_data_t *player)
{
    wiced_result_t                          result;

    result = wiced_ota2_service_deinit(player->ota2_bg_service);
    if (result != WICED_SUCCESS)
    {
        OTA2_APP_PRINT(OTA2_LOG_WARNING, ("wiced_ota2_service_deinit() returned:%d\r\n", result));
    }
    player->ota2_bg_service = NULL;
    return result;
}


/****************************************************************
 *  Application Main loop Function
 ****************************************************************/

static void ota2_test_mainloop(ota2_data_t *player)
{
    wiced_result_t      result;
    uint32_t            events;

    OTA2_APP_PRINT(OTA2_LOG_DEBUG, ("Begin ota2_test mainloop\r\n"));

    /*
     * If auto play is set then start off by sending ourselves a play event.
     */

    while (player->tag == PLAYER_TAG_VALID)
    {
        events = 0;

        result = wiced_rtos_wait_for_event_flags(&player->events, PLAYER_ALL_EVENTS, &events,
                                                 WICED_TRUE, WAIT_FOR_ANY_EVENT, WICED_WAIT_FOREVER);
        if (result != WICED_SUCCESS)
        {
            continue;
        }

        if (events & PLAYER_EVENT_SHUTDOWN)
        {
            OTA2_APP_PRINT(OTA2_LOG_DEBUG, ("mainloop received EVENT_SHUTDOWN\r\n"));
            break;
        }

        if (events & PLAYER_EVENT_DISCONNECT_AP)
        {
            OTA2_APP_PRINT(OTA2_LOG_DEBUG, ("mainloop received PLAYER_EVENT_CONNECT_OTA2_AP\r\n"));

            /* disconnect from current AP and connect to OTA2 update AP */
            /* drop the current network interface */
            result = wiced_ota2_network_down();
            if (result != WICED_SUCCESS)
            {
                OTA2_APP_PRINT(OTA2_LOG_ERROR, ("OTA2_SERVICE_CHECK_FOR_UPDATE: Bringing down network interface failed!\r\n"));
            }
        }

        if (events & PLAYER_EVENT_CONNECT_OTA2_AP)
        {
            OTA2_APP_PRINT(OTA2_LOG_DEBUG, ("mainloop received PLAYER_EVENT_CONNECT_OTA2_AP\r\n"));

            /* disconnect from current AP and connect to OTA2 update AP */
            result = wiced_ota2_network_up(&player->dct_app->ota2_update_ap_info);
            if (result != WICED_SUCCESS)
            {
                OTA2_APP_PRINT(OTA2_LOG_ERROR, ("OTA2_SERVICE_CHECK_FOR_UPDATE: Bringing up network interface failed!\r\n"));
            }
        }

        if (events & PLAYER_EVENT_CONNECT_DEFAULT_AP)
        {
            OTA2_APP_PRINT(OTA2_LOG_DEBUG, ("mainloop received PLAYER_EVENT_CONNECT_DEFAULT_AP\r\n"));

            /* Bring up the default network interface */
            result = wiced_ota2_network_up(&player->dct_wifi->stored_ap_list[0]);
            if (result != WICED_SUCCESS)
            {
                OTA2_APP_PRINT(OTA2_LOG_ERROR, ("PLAYER_EVENT_CONNECT_DEFAULT_AP: Bringing up network interface failed!\r\n"));
            }
        }

        if (events & PLAYER_EVENT_STATUS)
        {
            OTA2_APP_PRINT(OTA2_LOG_DEBUG, ("mainloop received connect STATUS\r\n"));
            if (player->ota2_bg_service != NULL)
            {
                if (wiced_ota2_service_status(player->ota2_bg_service) != WICED_SUCCESS)
                {
                    OTA2_APP_PRINT(OTA2_LOG_WARNING, ("OTA2 Service Info: No service running.\r\n"));
                }
            }
            else
            {
                OTA2_APP_PRINT(OTA2_LOG_WARNING, ("OTA2 Service Info: service not initialized !\r\n"));
            }
        }

        if (events & PLAYER_EVENT_GET_UPDATE)
        {
            OTA2_APP_PRINT(OTA2_LOG_ERROR, ("PLAYER_EVENT_GET_UPDATE ! \r\n"));
            result = ota2_test_get_update(player, OTA2_EXAMPLE_UPDATE_TYPE_NOW);
            if (result != WICED_SUCCESS)
            {
                    OTA2_APP_PRINT(OTA2_LOG_ERROR, ("PLAYER_EVENT_GET_UPDATE ota2_test_get_update() failed! %d \r\n", result));
            }
            else
            {
                OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("PLAYER_EVENT_GET_UPDATE ota2_test_get_update() done.\r\n"));
            }
        }

        if (events & PLAYER_EVENT_GET_TIMED_UPDATE)
        {
            OTA2_APP_PRINT(OTA2_LOG_ERROR, ("OTA2_EXAMPLE_UPDATE_TYPE_TIMED ! \r\n"));
            result = ota2_test_get_update(player, OTA2_EXAMPLE_UPDATE_TYPE_TIMED);
            if (result != WICED_SUCCESS)
            {
                    OTA2_APP_PRINT(OTA2_LOG_ERROR, ("PLAYER_EVENT_GET_TIMED_UPDATE Download setup failed! %d \r\n", result));
            }
            else
            {
                OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("PLAYER_EVENT_GET_TIMED_UPDATE function call ok, wait for timeout to start download.\r\n"));
            }
        }

        if (events & PLAYER_EVENT_STOP_TIMED_UPDATE)
        {
            result = ota2_test_stop_timed_update(player);
            OTA2_APP_PRINT(OTA2_LOG_ERROR, ("PLAYER_EVENT_STOP_TIMED_UPDATE called ota2_test_stop_timed_update()! %d \r\n", result));
            if (result != WICED_SUCCESS)
            {
                    OTA2_APP_PRINT(OTA2_LOG_ERROR, ("PLAYER_EVENT_STOP_TIMED_UPDATE Download failed! %d \r\n", result));
            }
            else
            {
                OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("PLAYER_EVENT_STOP_TIMED_UPDATE stopped download ! :) \r\n"));
            }
        }

        if (events & PLAYER_EVENT_FACTORY_RESET_STATUS)
        {
            OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("PLAYER_EVENT_FACTORY_RESET_STATUS Status validate\n"));
            wiced_ota2_image_validate ( WICED_OTA2_IMAGE_TYPE_FACTORY_RESET_APP );
        }

        if (events & PLAYER_EVENT_FACTORY_RESET_REBOOT)
        {
            OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("PLAYER_EVENT_FACTORY_RESET_REBOOT Reset on Reboot\n"));
            wiced_ota2_force_factory_reset_on_reboot();
        }

        if (events & PLAYER_EVENT_FACTORY_RESET_NOW)
        {
            /* extract the image in the staging area */
            OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("Extract the Factory Reset App NOW\r\n"));
            wiced_ota2_image_extract ( WICED_OTA2_IMAGE_TYPE_FACTORY_RESET_APP );
            OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("Reboot now to see change!\r\n"));
        }

        if (events & PLAYER_EVENT_UPDATE_NOW)
        {
            /* extract the image in the staging area */
            OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("Extract the update NOW\r\n"));
            result = wiced_dct_ota2_save_copy(OTA2_BOOT_UPDATE);
            OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("Update NOW - copied DCT:%d\r\n", result));
            wiced_ota2_image_fakery(WICED_OTA2_IMAGE_DOWNLOAD_COMPLETE);
            wiced_ota2_image_extract ( WICED_OTA2_IMAGE_TYPE_STAGED );
            OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("Reboot now to see change!\r\n"));
        }

        if (events & PLAYER_EVENT_UPDATE_REBOOT)
        {
            OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("Mark the update to be extracted on next Boot.\r\n"));
            wiced_ota2_image_fakery(WICED_OTA2_IMAGE_EXTRACT_ON_NEXT_BOOT);
            OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("ReBoot now to watch the extraction.\r\n"));
        }

        if (events & PLAYER_EVENT_UPDATE_STATUS)
        {
            OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("PLAYER_EVENT_UPDATE Status validate\n"));
            wiced_ota2_image_validate ( WICED_OTA2_IMAGE_TYPE_STAGED );
        }

    }   /* while */

    OTA2_APP_PRINT(OTA2_LOG_DEBUG, ("End ota2_test mainloop\r\n"));
}


static void ota2_test_shutdown(ota2_data_t *player)
{

    /*
     * Shutdown the console.
     */

    command_console_deinit();

    ota2_test_stop_timed_update(player);

    wiced_rtos_deinit_event_flags(&player->events);

    wiced_dct_read_unlock(player->dct_network, WICED_TRUE);
    wiced_dct_read_unlock(player->dct_wifi, WICED_TRUE);
    wiced_dct_read_unlock(player->dct_app, WICED_TRUE);

    player->tag = PLAYER_TAG_INVALID;
    free(player);
}

static void set_nvram_mac(void)
{
#ifdef WWD_TEST_NVRAM_OVERRIDE
    platform_dct_wifi_config_t dct_wifi;
    wiced_result_t result;
    uint32_t size;
    uint32_t i;
    char *nvram;

    result = wiced_dct_read_with_copy(&dct_wifi, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t));
    if (result != WICED_SUCCESS)
    {
        return;
    }

    if (wwd_bus_get_wifi_nvram_image(&nvram, &size) != WWD_SUCCESS)
    {
        return;
    }

    /*
     * We have the mac address from the DCT so now we just need to update the nvram image.
     * Search for the 'macaddr=' token.
     */

    for (i = 0; i < size; )
    {
        if (nvram[i] == '\0')
        {
            break;
        }

        if (nvram[i] != 'm' || nvram[i+1] != 'a' || nvram[i+2] != 'c' || nvram[i+3] != 'a' ||
            nvram[i+4] != 'd' || nvram[i+5] != 'd' || nvram[i+6] != 'r' || nvram[7] != '=')
        {
            while(i < size && nvram[i] != '\0')
            {
                i++;
            }
            i++;
            continue;
        }

        /*
         * Found the macaddr token. Now we just need to update it.
         */

        sprintf(&nvram[i+8], "%02x:%02x:%02x:%02x:%02x:%02x", dct_wifi.mac_address.octet[0],
                dct_wifi.mac_address.octet[1], dct_wifi.mac_address.octet[2], dct_wifi.mac_address.octet[3],
                dct_wifi.mac_address.octet[4], dct_wifi.mac_address.octet[5]);
        break;
    }
#endif
}


static ota2_data_t* init_player(void)
{
    ota2_data_t*            player = NULL;
    wiced_result_t          result;
    uint32_t                tag;
    ota2_boot_type_t        boot_type;

    tag = PLAYER_TAG_VALID;

    /*
     * Temporary to work around a WiFi bug with RMC.
     */

    set_nvram_mac();

    /* Initialize the device */
    result = wiced_init();
    if (result != WICED_SUCCESS)
    {
        return NULL;
    }

   /*
     * Allocate the main data structure.
     */
    player = calloc_named("ota2_test", 1, sizeof(ota2_data_t));
    if (player == NULL)
    {
        OTA2_APP_PRINT(OTA2_LOG_ALWAYS, ("Unable to allocate player structure\r\n"));
        return NULL;
    }

    player->log_level = OTA2_LOG_NOTIFY;

    /*
     * Create the command console.
     */

    OTA2_APP_PRINT(OTA2_LOG_INFO, ("Start the command console\r\n"));
    result = command_console_init(STDIO_UART, sizeof(ota2_command_buffer), ota2_command_buffer, CONSOLE_COMMAND_HISTORY_LENGTH, ota2_command_history_buffer, " ");
    if (result != WICED_SUCCESS)
    {
        OTA2_APP_PRINT(OTA2_LOG_DEBUG, ("Error starting the command console\r\n"));
        free(player);
        return NULL;
    }
    console_add_cmd_table(ota2_command_table);

    /*
     * Create our event flags.
     */
    result = wiced_rtos_init_event_flags(&player->events);
    if (result != WICED_SUCCESS)
    {
        OTA2_APP_PRINT(OTA2_LOG_DEBUG, ("Error initializing event flags\r\n"));
        tag = PLAYER_TAG_INVALID;
    }

    OTA2_APP_PRINT(OTA2_LOG_INFO, ("\nOTA2_IMAGE_FLASH_BASE                0x%08lx\n", (uint32_t)OTA2_IMAGE_FLASH_BASE));
    OTA2_APP_PRINT(OTA2_LOG_INFO, ("OTA2_IMAGE_FACTORY_RESET_AREA_BASE   0x%08lx\n", (uint32_t)OTA2_IMAGE_FACTORY_RESET_AREA_BASE));
    OTA2_APP_PRINT(OTA2_LOG_INFO, ("OTA2_IMAGE_APP_DCT_SAVE_AREA_BASE    0x%08lx\n", (uint32_t)OTA2_IMAGE_APP_DCT_SAVE_AREA_BASE));
    OTA2_APP_PRINT(OTA2_LOG_INFO, ("OTA2_IMAGE_CURRENT_AREA_BASE         0x%08lx\n", (uint32_t)OTA2_IMAGE_CURRENT_AREA_BASE));
    OTA2_APP_PRINT(OTA2_LOG_INFO, ("OTA2_IMAGE_CURR_LUT_AREA_BASE        0x%08lx\n", (uint32_t)OTA2_IMAGE_CURR_LUT_AREA_BASE));
    OTA2_APP_PRINT(OTA2_LOG_INFO, ("OTA2_IMAGE_CURR_DCT_1_AREA_BASE      0x%08lx\n", (uint32_t)OTA2_IMAGE_CURR_DCT_1_AREA_BASE));
    OTA2_APP_PRINT(OTA2_LOG_INFO, ("OTA2_IMAGE_CURR_FS_AREA_BASE         0x%08lx\n", (uint32_t)OTA2_IMAGE_CURR_FS_AREA_BASE));
    OTA2_APP_PRINT(OTA2_LOG_INFO, ("OTA2_IMAGE_CURR_APP0_AREA_BASE       0x%08lx\n", (uint32_t)OTA2_IMAGE_CURR_APP0_AREA_BASE));
#if defined(OTA2_IMAGE_LAST_KNOWN_GOOD_AREA_BASE)
    OTA2_APP_PRINT(OTA2_LOG_INFO, ("OTA2_IMAGE_LAST_KNOWN_GOOD_AREA_BASE 0x%08lx\n", (uint32_t)OTA2_IMAGE_LAST_KNOWN_GOOD_AREA_BASE));
#endif
    OTA2_APP_PRINT(OTA2_LOG_INFO, ("OTA2_IMAGE_STAGING_AREA_BASE         0x%08lx\r\n", (uint32_t)OTA2_IMAGE_STAGING_AREA_BASE));

    /* read in our configurations */
    /* network */
    wiced_dct_read_lock( (void**)&player->dct_network, WICED_TRUE, DCT_NETWORK_CONFIG_SECTION, 0, sizeof(platform_dct_network_config_t) );

    /* wifi */
    wiced_dct_read_lock((void**)&player->dct_wifi, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t));

    /* App */
    wiced_dct_read_lock( (void**)&player->dct_app, WICED_TRUE, DCT_APP_SECTION, 0, sizeof( ota2_dct_t ) );

    /* determine if this is a first boot, factory reset, or after an update boot */
    boot_type = wiced_ota2_get_boot_type();
    switch( boot_type )
    {
        case OTA2_BOOT_NEVER_RUN_BEFORE:
            OTA2_APP_PRINT(OTA2_LOG_INFO, ("First BOOT EVER\r\n"));
            /* Set the reboot type back to normal so we don't think we updated next reboot */
            wiced_dct_ota2_save_copy( OTA2_BOOT_NORMAL );
            break;
        case OTA2_BOOT_NORMAL:
            OTA2_APP_PRINT(OTA2_LOG_INFO, ("Normal reboot - count:%ld.\r\n", player->dct_app->reboot_count));
            break;
        case OTA2_BOOT_FACTORY_RESET:
            OTA2_APP_PRINT(OTA2_LOG_INFO, ("Factory Reset Occurred!\r\n"));
#if RESTORE_DCT_APP_SETTINGS
            over_the_air_2_app_restore_settings_after_update(player);
#endif
            /* Set the reboot type back to normal so we don't think we updated next reboot */
            wiced_dct_ota2_save_copy( OTA2_BOOT_NORMAL );
            break;
        case OTA2_BOOT_UPDATE:
            OTA2_APP_PRINT(OTA2_LOG_INFO, ("Update Occurred!\r\n"));
#if RESTORE_DCT_APP_SETTINGS
            over_the_air_2_app_restore_settings_after_update(player);
#endif
            /* Set the reboot type back to normal so we don't think we updated next reboot */
            wiced_dct_ota2_save_copy( OTA2_BOOT_NORMAL );
            break;
        case OTA2_SOFTAP_UPDATE:
            OTA2_APP_PRINT(OTA2_LOG_INFO, ("SoftAP Update Occurred!\r\n"));
#if RESTORE_DCT_APP_SETTINGS
            over_the_air_2_app_restore_settings_after_update(player);
#endif
            /* Set the reboot type back to normal so we don't think we updated next reboot */
            wiced_dct_ota2_save_copy( OTA2_BOOT_NORMAL );
            break;
        case OTA2_BOOT_LAST_KNOWN_GOOD:
            OTA2_APP_PRINT(OTA2_LOG_INFO, ("Last Known Good used!\r\n"));
            break;
    }

    /* Get RTC Clock time and set it here */
    {
        wiced_time_t time = 0;
        wiced_time_set_time( &time );
    }


    /* keep track of # of reboots */
    player->dct_app->reboot_count++;
    wiced_dct_write( (void*)player->dct_app, DCT_APP_SECTION, 0, sizeof( ota2_dct_t ) );

    /* print out our current configuration */
    ota2_config_print_info(player);

    /* connect to the network */
    result = wiced_ota2_network_up(&player->dct_wifi->stored_ap_list[0]);
    if (result != WICED_SUCCESS)
    {
        OTA2_APP_PRINT(OTA2_LOG_ERROR, ("init: Bringing up network interface failed!\r\n"));
    }

    /* set our valid tag */
    player->tag = tag;

    return player;
}


wiced_result_t over_the_air_2_app_restore_settings_after_update(ota2_data_t* player)
{
    platform_dct_network_config_t   dct_network = { 0 };
    platform_dct_wifi_config_t      dct_wifi = { 0 };
    ota2_dct_t                      dct_app = { 0 };

    /* read in our configurations from the DCT copy */
    /* network */
    if (wiced_dct_ota2_read_saved_copy( &dct_network, DCT_NETWORK_CONFIG_SECTION, 0, sizeof(platform_dct_network_config_t)) != WICED_SUCCESS)
    {
        OTA2_APP_PRINT(OTA2_LOG_WARNING, ("over_the_air_2_app_restore_settings_after_update() failed reading Network Config!\r\n"));
        return WICED_ERROR;
    }

    /* wifi */
    if (wiced_dct_ota2_read_saved_copy( &dct_wifi, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t)) != WICED_SUCCESS)
    {
        OTA2_APP_PRINT(OTA2_LOG_WARNING, ("over_the_air_2_app_restore_settings_after_update() failed reading WiFi Config!\r\n"));
        return WICED_ERROR;
    }

    /* App */
    if (wiced_dct_ota2_read_saved_copy( &dct_app, DCT_APP_SECTION, 0, sizeof(ota2_dct_t)) != WICED_SUCCESS)
    {
        OTA2_APP_PRINT(OTA2_LOG_WARNING, ("over_the_air_2_app_restore_settings_after_update() failed reading App Config!\r\n"));
        return WICED_ERROR;
    }

    memcpy(player->dct_network, &dct_network, sizeof(platform_dct_network_config_t));
    memcpy(player->dct_wifi, &dct_wifi, sizeof(platform_dct_wifi_config_t));
    memcpy(player->dct_app, &dct_app, sizeof(ota2_dct_t));

    /* now, save them all! */
    if (ota2_save_config(player) != WICED_SUCCESS)
    {
        OTA2_APP_PRINT(OTA2_LOG_WARNING, ("over_the_air_2_app_restore_settings_after_update() failed Saving Config!\r\n"));
        return WICED_ERROR;
    }

    OTA2_APP_PRINT(OTA2_LOG_NOTIFY, ("Restored saved Configuration!\r\n"));
    return WICED_SUCCESS;
}



