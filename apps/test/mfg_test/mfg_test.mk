#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_mfg_test

$(NAME)_SOURCES := mfg_test_init.c

ifneq ($(PLATFORM),$(filter $(PLATFORM), BCM943364WCD1 BCM94343WWCD1 BCM943438WCD1 BCM94343WWCD2 ))
$(NAME)_RESOURCES += firmware/$(WLAN_CHIP)/$(WLAN_CHIP)$(WLAN_CHIP_REVISION)-mfgtest.bin
else
# Set the WIFI firmware in multi application file system to point to firmware
MULTI_APP_WIFI_FIRMWARE   := resources/firmware/$(WLAN_CHIP)/$(WLAN_CHIP)$(WLAN_CHIP_REVISION)-mfgtest.bin
endif
NO_WIFI_FIRMWARE := YES

$(NAME)_COMPONENTS += test/wl_tool

# wl commands which dump a lot of data require big buffers.
GLOBAL_DEFINES   += WICED_PAYLOAD_MTU=8320

ifeq ($(PLATFORM),$(filter $(PLATFORM),BCM943909WCD1_3.B0 BCM943909WCD1_3.B1))
GLOBAL_DEFINES += TX_PACKET_POOL_SIZE=10 \
                  RX_PACKET_POOL_SIZE=10 \
                  PBUF_POOL_TX_SIZE=8 \
                  PBUF_POOL_RX_SIZE=8 \
                  WICED_ETHERNET_DESCNUM_TX=32 \
                  WICED_ETHERNET_DESCNUM_RX=8 \
                  WICED_ETHERNET_RX_PACKET_POOL_SIZE=32+WICED_ETHERNET_DESCNUM_RX
else
ifneq ($(PLATFORM),$(filter $(PLATFORM), BCM943362WCD4_LPCX1769))
GLOBAL_DEFINES   += TX_PACKET_POOL_SIZE=2 \
                    RX_PACKET_POOL_SIZE=2 \
                    PBUF_POOL_TX_SIZE=2 \
                    PBUF_POOL_RX_SIZE=2
endif
endif



# ENABLE for ethernet support
#$(NAME)_DEFINES   += MFG_TEST_ENABLE_ETHERNET_SUPPORT

INVALID_PLATFORMS := BCM943362WCD4_LPCX1769 \
                     BCM943362WCDA \
                     BCM9WCD2WLREFAD.BCM94334WLAGB \
                     BCM943909QT

ifeq ($(PLATFORM),$(filter $(PLATFORM),BCM943362WCD4_LPCX1769 BCM94334WLAGB BCM943909QT))
ifneq (yes,$(strip $(TESTER)))
$(error Platform not supported for Manufacturing test)
endif
endif

#==============================================================================
# iperf inclusion
#==============================================================================
#Disabled iperf support in the mfg_test app for the low memory platforms listed below; Enabled for all other platforms not listed
ifeq (,$(filter $(PLATFORM),BCM94390WCD1 BCM94390WCD2 BCM94390WCD3 BCM943364WCD1 BCM94343WWCD1 BCM94343WWCD2 BCM943438WCD1))
MFG_TEST_ENABLE_IPERF :=1
GLOBAL_DEFINES   += MFG_TEST_ENABLE_IPERF
endif

ifdef MFG_TEST_ENABLE_IPERF
$(NAME)_COMPONENTS += test/iperf
endif

#==============================================================================
# audio loopback inclusion
#==============================================================================

ifneq ($(filter $(PLATFORM),BCM943909WCD1_3.B0 BCM943909WCD1_3.B1 BCM943907WAE_1.B0 BCM943907WAE_1.B1 BCM943907APS.B0 BCM943907APS.B1 BCM943907WCD1 BCM943907WCD2),)
ifneq ($(filter $(RTOS)-$(NETWORK),ThreadX- ThreadX-NetX_Duo),)
MFG_TEST_ENABLE_AUDIO_LOOPBACK :=1
GLOBAL_DEFINES += MFG_TEST_ENABLE_AUDIO_LOOPBACK
endif
endif

ifdef MFG_TEST_ENABLE_AUDIO_LOOPBACK
$(NAME)_COMPONENTS += test/audio_loopback
GLOBAL_DEFINES     += WICED_AUDIO_LOOPBACK_LOG_ENABLED=0
endif

#==============================================================================
# Flag for app specific
#==============================================================================
GLOBAL_DEFINES   += MFG_TEST_APP
