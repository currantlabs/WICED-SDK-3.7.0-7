#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_apollo

APOLLO_TX_PACKET_POOL_SIZE ?= 10
APOLLO_RX_PACKET_POOL_SIZE ?= 10

# To include Dolby Digital Decoder support in the Apollo streamer library, uncomment the
# USE_UDC define or add USE_UDC=1 to the target build string. For example: demo.apollo-BCM943909WCD1_3.B0 USE_UDC=1
#USE_UDC := 1

#WICED_ENABLE_TRACEX := 1

# To build Apollo without Bluetooth, uncomment the APOLLO_NO_BT define.
#APOLLO_NO_BT := 1

ifdef APOLLO_NO_BT
$(info apollo building without BT)
GLOBAL_DEFINES     += APOLLO_NO_BT
endif

#GLOBAL_DEFINES     += CONSOLE_ENABLE_WL

GLOBAL_DEFINES     += APPLICATION_STACK_SIZE=8000

$(NAME)_SOURCES    := apollo.c
$(NAME)_SOURCES    += apollo_debug.c
$(NAME)_SOURCES    += apollo_config.c

$(NAME)_COMPONENTS := audio/apollo/audio_render \
                      audio/apollo/apollo_player \
                      audio/apollo/apollo_streamer \
                      audio/apollo/apollocore \
                      audio/display \
                      utilities/command_console \
                      utilities/command_console/wifi \
                      inputs/button_manager \
                      drivers/power_management

ifndef APOLLO_NO_BT
$(NAME)_COMPONENTS += audio/apollo/apollo_bt_service
endif

ifneq ($(filter $(PLATFORM),BCM943907WAE_1.B0 BCM943907WAE_1.B1 BCM943907WAE2_1.B1),)
    GLOBAL_DEFINES  += POWER_MANAGEMENT_ON_BCM943907WAE_1
endif

# Enable this for getting Bluetooth protocol traces
#GLOBAL_DEFINES     += ENABLE_BT_PROTOCOL_TRACES

# include display
# GLOBAL_DEFINES += USE_AUDIO_DISPLAY

ifneq (,$(findstring CONSOLE_ENABLE_WL,$(GLOBAL_DEFINES)))
$(NAME)_COMPONENTS += test/wl_tool
endif

APPLICATION_DCT    := apollo_dct.c

WIFI_CONFIG_DCT_H  := wifi_config_dct.h

# Bluetooth MAC address and name are configured via the DCT
BT_CONFIG_DCT_H    := bt_config_dct.h

ifdef WICED_ENABLE_TRACEX
$(info apollo using tracex lib)

#Only use DDR on WCD1 boards
ifneq ($(filter $(PLATFORM),BCM943909WCD1_3.B0 BCM943909WCD1_3.B1),)
GLOBAL_DEFINES     += WICED_TRACEX_BUFFER_DDR_OFFSET=0x0
GLOBAL_DEFINES     += WICED_TRACEX_BUFFER_SIZE=0x200000
else
GLOBAL_DEFINES     += WICED_TRACEX_BUFFER_SIZE=0x10000
endif

$(NAME)_COMPONENTS += test/TraceX
$(NAME)_SOURCES    += apollo_tracex.c
endif

GLOBAL_DEFINES     += TX_PACKET_POOL_SIZE=$(APOLLO_TX_PACKET_POOL_SIZE)
GLOBAL_DEFINES     += RX_PACKET_POOL_SIZE=$(APOLLO_RX_PACKET_POOL_SIZE)

GLOBAL_DEFINES     += WICED_USE_AUDIO

GLOBAL_DEFINES     += AUTO_IP_ENABLED

#GLOBAL_DEFINES     += WICED_DISABLE_WATCHDOG

VALID_OSNS_COMBOS  := ThreadX-NetX_Duo
VALID_PLATFORMS    :=
VALID_PLATFORMS    += BCM943909WCD*
VALID_PLATFORMS    += BCM943907*
INVALID_PLATFORMS  += BCM943907AEVAL* BCM943907WCD2
