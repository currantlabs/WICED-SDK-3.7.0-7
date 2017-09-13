#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
NAME := Platform_BCM943907WCD1

WLAN_CHIP_REVISION   := B0
APPS_CHIP_REVISION   := B1
WLAN_CHIP            := 43909
WLAN_CHIP_FAMILY     := 4390x
HOST_MCU_FAMILY      := BCM4390x
HOST_MCU_VARIANT     := BCM43907
HOST_MCU_PART_NUMBER := BCM43907WLCSP

# BCM943907WCD1 includes BCM20707 BT chip
BT_CHIP          := 20707
BT_CHIP_REVISION := A1
BT_CHIP_XTAL_FREQUENCY := 40MHz

PLATFORM_SUPPORTS_BUTTONS := 1

PLATFORM_NO_DDR := 1
PLATFORM_NO_GMAC := 1

GLOBAL_DEFINES += SFLASH_SUPPORT_MACRONIX_PARTS
GLOBAL_DEFINES += WICED_DCT_INCLUDE_BT_CONFIG

WICED_USB_SUPPORT := yes

WICED_BASE := ../../
PLATFORM_SOURCES := $(WICED_BASE)/platforms/$(PLATFORM_DIRECTORY)/

$(NAME)_SOURCES := platform.c

GLOBAL_INCLUDES := .

ifeq (,$(APP_WWD_ONLY)$(NS_WWD_ONLY)$(RTOS_WWD_ONLY))
$(NAME)_SOURCES += platform_audio.c \
                   wiced_audio.c
$(NAME)_COMPONENTS += drivers/audio/AK4954 \
                      drivers/audio/WM8533 \
                      drivers/audio/spdif
endif  # (,$(APP_WWD_ONLY)$(NS_WWD_ONLY)$(RTOS_WWD_ONLY))