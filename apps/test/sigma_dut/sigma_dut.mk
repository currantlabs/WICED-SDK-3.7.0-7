#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_sigma_dut_$(RTOS)_$(NETWORK)

$(NAME)_SOURCES := wiced_init.c \
                   wifi/wifi.c \
                   wifi_cert/wifi_cert_commands.c \
                   wifi_cert/$(RTOS)_wifi_cert_ping.c \
                   wifi_cert/wifi_cert_traffic_stream.c \
                   wifi_cert/wifi_cert_thread.c

$(NAME)_INCLUDES := .
$(NAME)_DEFINES  := NETWORK_$(NETWORK)



$(NAME)_COMPONENTS += utilities/command_console

GLOBAL_DEFINES += STDIO_BUFFER_SIZE=1024
GLOBAL_DEFINES += configUSE_MUTEXES=1

GLOBAL_DEFINES += DISABLE_LOGGING
#==============================================================================
# P2P inclusion
# Uncomment the following lines if testing P2P
#==============================================================================
#WICED_USE_WIFI_P2P_INTERFACE = 1
#GLOBAL_DEFINES += WICED_USE_WIFI_P2P_INTERFACE
#GLOBAL_DEFINES += WICED_DCT_INCLUDE_P2P_CONFIG

#==============================================================================
# Platform specific settings.
#==============================================================================
# Increase packet pool size for particular platforms.  Note that the packet pools need to be large enough to handle a 30000 byte ping if running the standard 11n test plan.
ifeq ($(PLATFORM),$(filter $(PLATFORM), BCM943362WCD4 BCM94390WCD2 ))
GLOBAL_DEFINES += TX_PACKET_POOL_SIZE=10 \
                  RX_PACKET_POOL_SIZE=10 \
                  WICED_TCP_TX_DEPTH_QUEUE=8 \
                  TCP_WINDOW_SIZE=8192
else
# Set TX and RX pool length for 4343WWCD2
# Added this separately because 4343WWCD2 needs atleast ~14 TX pools to be able to send 10k Tx ping size and at least 20 RX buffers to support 30k Rx ping size
ifeq ($(PLATFORM),$(filter $(PLATFORM), BCM94343WWCD2))
GLOBAL_DEFINES += TX_PACKET_POOL_SIZE=22 \
                  RX_PACKET_POOL_SIZE=22 \
                  WICED_TCP_TX_DEPTH_QUEUE=8 \
                  TCP_WINDOW_SIZE=8192
else
# Set 43909 specific packet pool settings
BCM94390x_PLATFORMS := BCM943909* BCM943907* BCM943903*
$(eval BCM94390x_PLATFORMS := $(call EXPAND_WILDCARD_PLATFORMS,$(BCM94390x_PLATFORMS)))
ifeq ($(PLATFORM),$(filter $(PLATFORM), $(BCM94390x_PLATFORMS)))
GLOBAL_DEFINES += TX_PACKET_POOL_SIZE=60 \
                  RX_PACKET_POOL_SIZE=60 \
                  WICED_TCP_TX_DEPTH_QUEUE=8 \
                  WICED_ETHERNET_DESCNUM_TX=32 \
                  WICED_ETHERNET_DESCNUM_RX=16 \
                  WICED_ETHERNET_RX_PACKET_POOL_SIZE=40+WICED_ETHERNET_DESCNUM_RX \
                  TCP_WINDOW_SIZE=8192
else
# Otherwise use default values
endif
endif
endif
