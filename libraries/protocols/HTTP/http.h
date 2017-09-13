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

#include "wiced.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *            Enumerations
 ******************************************************/

#define SIMPLE_GET_REQUEST \
    "GET /get HTTP/1.1\r\n" \
    "Host: www.httpbin.org\r\n" \
    "Connection: close\r\n" \
    "\r\n"

/******************************************************
 *             Structures
 ******************************************************/

/******************************************************
 *             Function declarations
 ******************************************************/

wiced_result_t wiced_http_get ( wiced_ip_address_t* address, const char* query, void* buffer, uint32_t buffer_length );
wiced_result_t wiced_https_get( wiced_ip_address_t* address, const char* query, void* buffer, uint32_t buffer_length, const char* peer_cn );
wiced_result_t wiced_https_get_with_specified_port( wiced_ip_address_t* address, const char* query, void* buffer, uint32_t buffer_length, uint16_t port, const char* peer_cn );

#ifdef __cplusplus
} /* extern "C" */
#endif
