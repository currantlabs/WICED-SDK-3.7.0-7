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


/******************************************************
 *                      Macros
 ******************************************************/

#if defined(TX_ENABLE_EVENT_TRACE)

#define APOLLO_TRACEX_COMMANDS \
    { (char*) "tracex", apollo_tracex_command, 1, NULL, NULL, (char *)"[<command> [option...]]", (char *)"Apollo TraceX commands" }, \

#else

#define APOLLO_TRACEX_COMMANDS

#endif

/******************************************************
 *                     Macros
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
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

int apollo_tracex_command(int argc, char *argv[]);

#ifdef __cplusplus
} /* extern "C" */
#endif
