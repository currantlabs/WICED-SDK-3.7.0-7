#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_Bluetooth_Audio

$(NAME)_SOURCES    := bluetooth_audio.c \
                      bluetooth_audio_player.c \
                      bluetooth_audio_nv.c \
                      bluetooth_audio_decoder.c

$(NAME)_COMPONENTS := libraries/drivers/bluetooth/dual_mode \
                      libraries/audio/codec/codec_framework \
                      libraries/audio/codec/sbc_if \
                      utilities/command_console

BT_CONFIG_DCT_H := bt_config_dct.h

GLOBAL_DEFINES := BUILDCFG \
                  WICED_USE_AUDIO \
                  WICED_NO_WIFI \
                  NO_WIFI_FIRMWARE \
                  TX_PACKET_POOL_SIZE=1 \
                  RX_PACKET_POOL_SIZE=1 \
                  USE_MEM_POOL

#GLOBAL_DEFINES += WICED_DISABLE_WATCHDOG

ifneq (,$(findstring USE_MEM_POOL,$(GLOBAL_DEFINES)))
$(NAME)_SOURCES   += mem_pool/mem_pool.c
$(NAME)_INCLUDES  := ./mem_pool
#GLOBAL_DEFINES    += MEM_POOL_DEBUG
endif

ifneq ($(filter $(PLATFORM),BCM943909WCD1_3.B0 BCM943909WCD1_3.B1 BCM943909B0FCBU BCM943907WAE_1.B0 BCM943907WAE_1.B1 BCM943907WAE2_1.B1 BCM943907WAE2_1.B1 BCM943907APS.B0 BCM943907APS.B1 BCM943907WCD1 BCM943907WCD1.B1),)
# Use h/w audio PLL to tweak the master clock going into the I2S engine
GLOBAL_DEFINES     += USE_AUDIO_PLL
$(NAME)_SOURCES    += bluetooth_audio_pll_tuning.c
$(NAME)_COMPONENTS += audio/apollo/audio_pll_tuner \
                      audio/apollo/apollocore
endif

# Define ENABLE_BT_PROTOCOL_TRACES to enable Bluetooth protocol/profile level
# traces.
#GLOBAL_DEFINES     += ENABLE_BT_PROTOCOL_TRACES

VALID_PLATFORMS    := BCM9WCD1AUDIO BCM943909* BCM943907WAE* BCM943907APS* BCM943907WCD1
INVALID_PLATFORMS  := BCM943909QT BCM943907WCD2

NO_WIFI_FIRMWARE   := YES
