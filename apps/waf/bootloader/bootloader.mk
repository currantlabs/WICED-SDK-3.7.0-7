#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_WICED_Bootloader_$(PLATFORM)

$(NAME)_SOURCES    := bootloader.c

NoOS_FIQ_STACK     := 0
NoOS_IRQ_STACK     := 256
NoOS_SYS_STACK     := 0

APP_WWD_ONLY       := 1
NO_WIFI_FIRMWARE   := YES
NO_WIFI            := YES
NO_WICED_API       := 1

GLOBAL_DEFINES     := WICED_NO_WIFI
GLOBAL_DEFINES     += WICED_DISABLE_STDIO
GLOBAL_DEFINES     += WICED_DISABLE_MCU_POWERSAVE
GLOBAL_DEFINES     += WICED_DCACHE_WTHROUGH
GLOBAL_DEFINES     += NO_WIFI_FIRMWARE
GLOBAL_DEFINES     += BOOTLOADER
# stack needs to be big enough to handle the CRC32 calculation buffer
ifneq ($(filter $(PLATFORM),BCM943364WCDA BCM943362WCD4),)
NoOS_START_STACK   := 6800
GLOBAL_DEFINES     += DCT_CRC32_CALCULATION_SIZE_ON_STACK=128
else
NoOS_START_STACK   := 8000
GLOBAL_DEFINES     += DCT_CRC32_CALCULATION_SIZE_ON_STACK=1024
endif

GLOBAL_INCLUDES    += .
GLOBAL_INCLUDES    += ../../../libraries/utilities/linked_list

$(NAME)_COMPONENTS := filesystems/wicedfs utilities/crc

VALID_OSNS_COMBOS  := NoOS
VALID_BUILD_TYPES  := release
