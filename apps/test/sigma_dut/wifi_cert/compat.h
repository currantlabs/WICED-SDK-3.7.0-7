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

#if defined(NETWORK_NetX)
#include "tx_api.h"
#include "nx_api.h"
#include "netx_applications/dns/nx_dns.h"
#elif defined(NETWORK_NetX_Duo)
#include "tx_api.h"
#include "nx_api.h"
#include "netx_applications/dns/nxd_dns.h"
#elif defined(NETWORK_LwIP)
#include "lwip/sockets.h"
#include "lwip/init.h"
#include "lwip/tcpip.h"
#include "lwip/inet.h"
#include "lwip/sys.h"
#endif

#ifdef __cplusplus
}
#endif
