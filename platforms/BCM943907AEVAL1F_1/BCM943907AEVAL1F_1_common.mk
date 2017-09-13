#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
WLAN_CHIP            := 43909
WLAN_CHIP_FAMILY     := 4390x
HOST_MCU_FAMILY      := BCM4390x
HOST_MCU_VARIANT     := BCM43907
HOST_MCU_PART_NUMBER := BCM43907WLCSPR

PLATFORM_SUPPORTS_BUTTONS := 1

PLATFORM_NO_USB_HOST := 1
PLATFORM_NO_DDR      := 1

GLOBAL_DEFINES += SFLASH_SUPPORT_MACRONIX_PARTS

PLATFORM_SOURCES := $(SOURCE_ROOT)platforms/$(PLATFORM_DIRECTORY)/../

$(NAME)_SOURCES := ../platform.c

GLOBAL_INCLUDES := . \
                   ..