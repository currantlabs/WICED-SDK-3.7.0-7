/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file Apollo TraceX support routines
 *
 */

#include "apollo_log.h"
#include "command_console.h"

#if defined(TX_ENABLE_EVENT_TRACE)

#include "TraceX.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define APOLLO_TRACEX_TCP_SERVER_IP          MAKE_IPV4_ADDRESS(192,168,0,200)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    APOLLO_TRACEX_CMD_ENABLE,
    APOLLO_TRACEX_CMD_DISABLE,
    APOLLO_TRACEX_CMD_STATUS,
    APOLLO_TRACEX_CMD_SEND,
    APOLLO_TRACEX_CMD_HELP,

    APOLLO_TRACEX_CMD_MAX,
} APOLLO_TRACEX_CMD_T;

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

static char* apollo_tracex_cmds[APOLLO_TRACEX_CMD_MAX] =
{
    "enable",
    "disable",
    "status",
    "send",
    "help"
};

/******************************************************
 *               Function Definitions
 ******************************************************/

static int apollo_tracex_send(int argc, char *argv[])
{
    wiced_result_t        result;
    wiced_tracex_config_t config;

    if (wiced_tracex_config_get(&config) != WICED_SUCCESS)
    {
        apollo_log_msg(APOLLO_LOG_ERR, "Failed to get TraceX configuration\n");
        return ERR_UNKNOWN;
    }

    if (argc > 2)
    {
        uint32_t a, b, c, d;

        /*
         * Use the specified an IP address.
         */

        if (sscanf(argv[2], "%lu.%lu.%lu.%lu", &a, &b, &c, &d) != 4)
        {
            apollo_log_msg(APOLLO_LOG_ERR, "Invalid TCP server IP address\n");
            return ERR_INSUFFICENT_ARGS;
        }

        SET_IPV4_ADDRESS(config.tcp_server.ip, MAKE_IPV4_ADDRESS(a, b, c, d));
    }

    apollo_log_msg(APOLLO_LOG_INFO, "Sending TraceX buffer to TCP server (%lu.%lu.%lu.%lu:%u)\n",
                     (GET_IPV4_ADDRESS(config.tcp_server.ip) >> 24) & 0xFF,
                     (GET_IPV4_ADDRESS(config.tcp_server.ip) >> 16) & 0xFF,
                     (GET_IPV4_ADDRESS(config.tcp_server.ip) >> 8) & 0xFF,
                     (GET_IPV4_ADDRESS(config.tcp_server.ip) >> 0) & 0xFF,
                     config.tcp_server.port);

    result = wiced_tracex_buf_send(&config.tcp_server);
    if (result == WICED_BADVALUE)
    {
        apollo_log_msg(APOLLO_LOG_ERR, "Cannot send TraceX buffer while TraceX is enabled\n");
        return ERR_UNKNOWN;
    }
    else if (result == WICED_NOT_CONNECTED)
    {
        apollo_log_msg(APOLLO_LOG_ERR, "Failed to connect to TCP server\n");
        return ERR_UNKNOWN;
    }
    else if (result != WICED_SUCCESS)
    {
        apollo_log_msg(APOLLO_LOG_ERR, "Failed to send TraceX buffer\n");
        return ERR_UNKNOWN;
    }

    return ERR_CMD_OK;
}


static int apollo_tracex_status(void)
{
    wiced_tracex_config_t config;
    wiced_tracex_status_t status;
    char                  str[WICED_TRACEX_FILTER_MAX_STRING_LEN];

    if (wiced_tracex_status_get(&status) != WICED_SUCCESS)
    {
        apollo_log_msg(APOLLO_LOG_ERR, "Failed to get TraceX status\n");
        return ERR_UNKNOWN;
    }

    if (wiced_tracex_config_get(&config) != WICED_SUCCESS)
    {
        apollo_log_msg(APOLLO_LOG_ERR, "Failed to get TraceX configuration\n");
        return ERR_UNKNOWN;
    }

    if (wiced_tracex_filter_mask_to_str(config.filter, str, sizeof(str)) != WICED_SUCCESS)
    {
        apollo_log_msg(APOLLO_LOG_ERR, "Failed to get TraceX filters\n");
        return ERR_UNKNOWN;
    }

    apollo_log_printf("TraceX state   : ");
    switch (status.state)
    {
        case WICED_TRACEX_STATE_DISABLED:
            apollo_log_printf("Disabled\n");
            break;

        case WICED_TRACEX_STATE_DISABLED_BUF_FULL:
            apollo_log_printf("Disabled (buffer full)\n");
            break;

        case WICED_TRACEX_STATE_DISABLED_TCP_ERR:
            apollo_log_printf("Disabled (TCP error)\n");
            break;

        case WICED_TRACEX_STATE_DISABLED_TRACEX_ERR:
            apollo_log_printf("Disabled (TraceX error)\n");
            break;

        case WICED_TRACEX_STATE_ENABLED:
            apollo_log_printf("Enabled\n");
            break;

        default:
            apollo_log_printf("Unknown\n");
            break;
    }

    apollo_log_printf("Buffer         : %u bytes @ %p with %lu objects\n",
                      config.buf.size, config.buf.addr, config.buf.obj_cnt);
    apollo_log_printf("Loop recording : %s\n",
                      config.loop_rec == WICED_TRUE ? "Enabled" : "Disabled");
    apollo_log_printf("Filters        : '%s' (0x%08lx)\n", str, config.filter);
    apollo_log_printf("TCP server     : %lu.%lu.%lu.%lu:%u (%s); max data length of %lu bytes\n",
                      (GET_IPV4_ADDRESS(config.tcp_server.ip) >> 24) & 0xFF,
                      (GET_IPV4_ADDRESS(config.tcp_server.ip) >> 16) & 0xFF,
                      (GET_IPV4_ADDRESS(config.tcp_server.ip) >> 8) & 0xFF,
                      (GET_IPV4_ADDRESS(config.tcp_server.ip) >> 0) & 0xFF,
                      config.tcp_server.port,
                      config.tcp_server.enable == WICED_FALSE ? "Disabled" :
                      status.tcp_connected == WICED_TRUE ? "Connected" :
                      "Disconnected", config.tcp_server.max_data_len);

    return ERR_CMD_OK;
}


static int apollo_tracex_disable(void)
{
    wiced_tracex_status_t status;

    if (wiced_tracex_status_get(&status) != WICED_SUCCESS)
    {
        apollo_log_msg(APOLLO_LOG_ERR, "Failed to get TraceX status\n");
        return ERR_UNKNOWN;
    }

    if (status.state < WICED_TRACEX_STATE_ENABLED)
    {
        apollo_log_msg(APOLLO_LOG_ERR, "TraceX not running\n");
        return ERR_CMD_OK;
    }

    apollo_log_msg(APOLLO_LOG_INFO, "Disabling TraceX\n");
    if (wiced_tracex_disable() != WICED_SUCCESS)
    {
        apollo_log_msg(APOLLO_LOG_ERR, "Failed to disable TraceX\n");
        return ERR_UNKNOWN;
    }

    return ERR_CMD_OK;
}


static int apollo_tracex_enable(int argc, char *argv[])
{
    wiced_result_t        result;
    wiced_tracex_config_t config;

    /* Start with current config and just change the things we care about */
    if (wiced_tracex_config_get(&config) != WICED_SUCCESS )
    {
        apollo_log_msg(APOLLO_LOG_ERR, "Failed to get TraceX configuration\n");
        return ERR_UNKNOWN;
    }

    /*
     * Configure the TCP server address. When the TraceX buffer is full, it will be automatically
     * sent to the TCP server.
     */

    config.loop_rec          = WICED_FALSE;
    config.tcp_server.enable = WICED_TRUE;

    if (argc > 2)
    {
        uint32_t a, b, c, d;

        /*
         * Looks like the caller specified an IP address.
         */

        if (sscanf(argv[2], "%lu.%lu.%lu.%lu", &a, &b, &c, &d) != 4)
        {
            apollo_log_msg(APOLLO_LOG_ERR, "Invalid TCP server IP address\n");
            return ERR_INSUFFICENT_ARGS;
        }

        SET_IPV4_ADDRESS(config.tcp_server.ip, MAKE_IPV4_ADDRESS(a, b, c, d));
    }
    else
    {
        /*
         * Use the default IP address.
         */

        SET_IPV4_ADDRESS(config.tcp_server.ip, APOLLO_TRACEX_TCP_SERVER_IP);
    }

    apollo_log_msg(APOLLO_LOG_INFO, "Enabling TraceX\n");
    result = wiced_tracex_enable(&config);

    if (result == WICED_NOT_CONNECTED)
    {
        apollo_log_msg(APOLLO_LOG_ERR, "Failed to connect to TCP server (%lu.%lu.%lu.%lu:%u)\n",
                       (GET_IPV4_ADDRESS(config.tcp_server.ip) >> 24) & 0xFF, (GET_IPV4_ADDRESS(config.tcp_server.ip) >> 16) & 0xFF,
                       (GET_IPV4_ADDRESS(config.tcp_server.ip) >> 8) & 0xFF,  (GET_IPV4_ADDRESS(config.tcp_server.ip) >> 0) & 0xFF, config.tcp_server.port);
        return ERR_UNKNOWN;
    }
    else if (result == WICED_BADVALUE)
    {
        apollo_log_msg(APOLLO_LOG_ERR, "TraceX was already enabled\n");
        return ERR_UNKNOWN;
    }
    else if (result != WICED_SUCCESS)
    {
        apollo_log_msg(APOLLO_LOG_ERR, "Failed to enable TraceX\n");
        return ERR_UNKNOWN;
    }

    return ERR_CMD_OK;
}


int apollo_tracex_command(int argc, char *argv[])
{
    int i;

    if (argc < 2)
    {
        apollo_log_msg(APOLLO_LOG_ERR, "No TraceX command specified\n");
        return ERR_CMD_OK;
    }

    /*
     * Look the command requested.
     */

    for (i = 0; i < APOLLO_TRACEX_CMD_MAX; i++)
    {
        if (!strcasecmp(argv[1], apollo_tracex_cmds[i]))
        {
            break;
        }
    }

    if (i >= APOLLO_TRACEX_CMD_MAX)
    {
        apollo_log_msg(APOLLO_LOG_ERR, "Unrecognized TraceX command: %s\n", argv[1]);
        return ERR_CMD_OK;
    }

    switch (i)
    {
        case APOLLO_TRACEX_CMD_ENABLE:
            apollo_tracex_enable(argc, argv);
            break;

        case APOLLO_TRACEX_CMD_DISABLE:
            apollo_tracex_disable();
            break;

        case APOLLO_TRACEX_CMD_STATUS:
            apollo_tracex_status();
            break;

        case APOLLO_TRACEX_CMD_SEND:
            apollo_tracex_send(argc, argv);
            break;

        case APOLLO_TRACEX_CMD_HELP:
            apollo_log_printf("Apollo TraceX commands:\n");
            for (i = 0; i < APOLLO_TRACEX_CMD_MAX; i++)
            {
                apollo_log_printf("  %s\n", apollo_tracex_cmds[i]);
            }
            break;

        default:
            break;
    }

    return ERR_CMD_OK;
}

#endif
