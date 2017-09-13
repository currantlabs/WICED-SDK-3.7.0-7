# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.$


NAME := App_Rfmon

$(NAME)_SOURCES := rfmon.c

GLOBAL_DEFINES += INCLUDE_DISPLAY
GLOBAL_DEFINES += USE_CONSOLE

$(NAME)_COMPONENTS := graphics/u8g
$(NAME)_COMPONENTS += utilities/command_console
$(NAME)_COMPONENTS += utilities/command_console/wps \
                      utilities/command_console/wifi \
                      utilities/command_console/thread \
                      utilities/command_console/ping \
                      utilities/command_console/platform \
                      utilities/command_console/tracex \
                      utilities/command_console/mallinfo

GLOBAL_DEFINES += STDIO_BUFFER_SIZE=128

WIFI_CONFIG_DCT_H := wifi_config_dct.h

VALID_OSNS_COMBOS  := ThreadX-NetX_Duo
VALID_PLATFORMS    := BCM943907*
INVALID_PLATFORMS  := BCM943907AEVAL*
