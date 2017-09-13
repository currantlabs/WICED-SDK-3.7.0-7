#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_console

#==============================================================================
# Console specific files
#==============================================================================
$(NAME)_SOURCES := wiced_init.c

$(NAME)_COMPONENTS += utilities/command_console

#==============================================================================
# Additional command console modules
#==============================================================================
$(NAME)_COMPONENTS += utilities/command_console/wps \
                      utilities/command_console/wifi \
                      utilities/command_console/thread \
                      utilities/command_console/ping \
                      utilities/command_console/platform \
                      utilities/command_console/tracex \
                      utilities/command_console/mallinfo

ifneq ($(PLATFORM),$(filter $(PLATFORM), BCM94390WCD1 BCM94390WCD2 BCM94390WCD3 BCM943341WAE BCM943341WCD1 ))
$(NAME)_COMPONENTS           += utilities/command_console/p2p
WICED_USE_WIFI_P2P_INTERFACE := 1
$(NAME)_DEFINES              += CONSOLE_INCLUDE_P2P
GLOBAL_DEFINES               += WICED_DCT_INCLUDE_P2P_CONFIG
endif

BCM94390x_ETHERNET_PLATFORMS := BCM943909* BCM943907*
$(eval BCM94390x_ETHERNET_PLATFORMS := $(call EXPAND_WILDCARD_PLATFORMS,$(BCM94390x_ETHERNET_PLATFORMS)))
ifeq ($(PLATFORM),$(filter $(PLATFORM), $(BCM94390x_ETHERNET_PLATFORMS)))
$(NAME)_COMPONENTS += utilities/command_console/ethernet
$(NAME)_DEFINES += CONSOLE_INCLUDE_ETHERNET
endif

#==============================================================================
# Includes
#==============================================================================
$(NAME)_INCLUDES := .

#==============================================================================
# Configuration
#==============================================================================

#==============================================================================
# Global defines
#==============================================================================
GLOBAL_DEFINES += STDIO_BUFFER_SIZE=128

# Increase packet pool size for particular platforms

ifeq ($(PLATFORM),$(filter $(PLATFORM), BCM943364WCD1 BCM94343WWCD1 BCM94343WWCD2 BCM943438WCD1))
GLOBAL_DEFINES += TX_PACKET_POOL_SIZE=14 \
                  RX_PACKET_POOL_SIZE=10 \
                  WICED_TCP_TX_DEPTH_QUEUE=13 \
                  TCP_WINDOW_SIZE=131072
else
ifeq ($(PLATFORM),$(filter $(PLATFORM), BCM943362WCD4 BCM94390WCD2 ))
# Increased the TX packet pool size for BCM943362WCD4 to 15 from 10 as TX was failing
# for large ICMP(ping) packets of size 10000. Kindly restrain from lowering this value
# until you are certain
GLOBAL_DEFINES += TX_PACKET_POOL_SIZE=15 \
                  RX_PACKET_POOL_SIZE=10 \
                  WICED_TCP_TX_DEPTH_QUEUE=8 \
                  TCP_WINDOW_SIZE=8192
else
# Set 43909 specific packet pool settings
BCM94390x_PLATFORMS := BCM943909* BCM943907* BCM943903*
$(eval BCM94390x_PLATFORMS := $(call EXPAND_WILDCARD_PLATFORMS,$(BCM94390x_PLATFORMS)))
ifeq ($(PLATFORM),$(filter $(PLATFORM), $(BCM94390x_PLATFORMS)))
GLOBAL_DEFINES += TX_PACKET_POOL_SIZE=40 \
                  RX_PACKET_POOL_SIZE=40 \
                  WICED_TCP_TX_DEPTH_QUEUE=32 \
                  WICED_ETHERNET_DESCNUM_TX=32 \
                  WICED_ETHERNET_DESCNUM_RX=8 \
                  WICED_ETHERNET_RX_PACKET_POOL_SIZE=40+WICED_ETHERNET_DESCNUM_RX \
                  TCP_WINDOW_SIZE=32768
# Enlarge stack size of test.console for solving WPA-Enterprise issue of BCM4390x.
GLOBAL_DEFINES += CONSOLE_THREAD_STACK_SIZE=8*1024
else
# Otherwise use default values
endif
endif
endif

INVALID_PLATFORMS := BCM943362WCD4_LPCX1769 \
                     BCM943362WCDA \
                     STMDiscovery411_BCM43438

#==============================================================================
# Wl tool inclusion
#==============================================================================
# Platforms & combinations with enough memory to fit WL tool, can declare CONSOLE_ENABLE_WL := 1
CONSOLE_ENABLE_WL ?= 0
# Enable wl commands which require large data buffer. ex: wl curpower, wl dump ampdu, ...
# set WL_COMMAND_LARGE_BUFFER=1 to enable this. Default is disabled.
WL_COMMAND_LARGE_BUFFER ?= 0

ifeq ($(CONSOLE_ENABLE_WL),1)
ifeq ($(WL_COMMAND_LARGE_BUFFER),1)
# Some wl command might still not work when WICED_PAYLOAD_MTU=2560 (ex: scanresults might fail when there're many AP)
# Increasing WICED_PAYLOAD_MTU will increase the total packet buffer pools size and might not fit into the available ram size.
# 2560 is chosen for 1) selected wl commands <ex: curpower, dump ampdu, ...>, 2) acceptable packet pools size.
GLOBAL_DEFINES += WICED_PAYLOAD_MTU=2560
endif
endif

#==============================================================================
# Provision to replace wlan production fw with mfg_test FW
#==============================================================================
CONSOLE_USE_MFG_TEST_FW ?= 0
ifeq ($(CONSOLE_USE_MFG_TEST_FW),1)
ifneq ($(PLATFORM),$(filter $(PLATFORM), BCM943364WCD1 BCM94343WWCD1 BCM943438WCD1 BCM94343WWCD2))
$(NAME)_RESOURCES += firmware/$(WLAN_CHIP)/$(WLAN_CHIP)$(WLAN_CHIP_REVISION)-mfgtest.bin
else
# Set the WIFI firmware in multi application file system to point to firmware
MULTI_APP_WIFI_FIRMWARE   := resources/firmware/$(WLAN_CHIP)/$(WLAN_CHIP)$(WLAN_CHIP_REVISION)-mfgtest.bin
endif
NO_WIFI_FIRMWARE := YES
endif

#==============================================================================
# Network stack-specific inclusion
#==============================================================================
ifeq ($(NETWORK),NetX)
#$(NAME)_SOURCES += NetX/netdb.c

ifdef CONSOLE_ENABLE_WPS
GLOBAL_DEFINES  += ADD_NETX_EAPOL_SUPPORT
endif
endif

ifeq ($(NETWORK),NetX_Duo)
#$(NAME)_SOURCES += NetX_Duo/netdb.c

ifdef CONSOLE_ENABLE_WPS
GLOBAL_DEFINES  += ADD_NETX_EAPOL_SUPPORT
endif
endif

GLOBAL_DEFINES += CONSOLE_ENABLE_THREADS

#==============================================================================
# iperf inclusion
#==============================================================================
#4390 does not have enough memory for iperf
ifneq (,$(filter $(PLATFORM),BCM94390WCD1 BCM94390WCD2 BCM94390WCD3))
CONSOLE_NO_IPERF :=1
endif

ifndef CONSOLE_NO_IPERF
$(NAME)_COMPONENTS += test/iperf
$(NAME)_DEFINES    += CONSOLE_ENABLE_IPERF
endif

#==============================================================================
# Traffic generation inclusion
#==============================================================================
#$(NAME)_COMPONENTS += utilities/command_console/traffic_generation

#Optional Phyrate Logging
ifeq ($(RVR_PHYRATE_LOGGING),1)
GLOBAL_DEFINES     += RVR_PHYRATE_LOGGING
endif
