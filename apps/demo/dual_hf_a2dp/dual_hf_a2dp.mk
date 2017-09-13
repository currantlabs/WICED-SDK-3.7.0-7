#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_dual_hf_a2dp

$(NAME)_SOURCES    := dual_a2dp_hfp_audio.c \
                      bluetooth_dm.c \
                      bluetooth_nv.c \
                      app_dct.c \
                      bluetooth_a2dp.c \
                      bluetooth_hfp.c \
                      bluetooth_audio.c \
                      bluetooth_audio_decoder.c \
                      bluetooth_audio_player.c \
                      bluetooth_audio_recorder.c \
                      bluetooth_audio_common.c \
                      hashtable.c

$(NAME)_COMPONENTS := daemons/airplay_server \
                      daemons/Gedday \
                      mfi/mfi_sap \
                      protocols/HTTP \
                      libraries/drivers/bluetooth/dual_mode \
                      libraries/audio/tone_player \
                      libraries/audio/codec/codec_framework \
                      libraries/audio/codec/sbc_if

# Add button management on supported platforms
ifneq ($(filter $(PLATFORM),BCM943909WCD1_3.B0 BCM943909WCD1_3.B1 BCM943907WAE_1.B0 BCM943907WAE_1.B1 BCM943907APS.B0 BCM943907APS.B1 BCM943907WCD1 BCM943907WAE2_1.B1),)
$(NAME)_COMPONENTS += inputs/button_manager
$(NAME)_SOURCES    += app_keypad.c
endif

ifneq (,$(findstring BCM943907WAE_1,$(PLATFORM)))
# Option to use ssd1306 128x64 i2c display over WICED_I2C_2
$(info *** Dual A2DP_audio using SSD1306 display on WAE_1! ***)
GLOBAL_DEFINES     += USE_AUDIO_DISPLAY
GLOBAL_DEFINES     += USE_NO_WIFI
$(NAME)_COMPONENTS += audio/display
endif

BT_CONFIG_DCT_H  := bt_config_dct.h

APPLICATION_DCT    := app_dct.c

ifneq ($(filter $(PLATFORM),BCM943909B0FCBU BCM943907WAE_1.B0 BCM943907WAE_1.B1 BCM943907APS.B0 BCM943907APS.B1),)
GLOBAL_DEFINES     += TX_PACKET_POOL_SIZE=6 \
                      RX_PACKET_POOL_SIZE=16
else
GLOBAL_DEFINES     += TX_PACKET_POOL_SIZE=5 \
                      RX_PACKET_POOL_SIZE=10
endif

GLOBAL_DEFINES     += WICED_USE_AUDIO \
                      BUILDCFG \
                      APPLICATION_STACK_SIZE=14336 \

# Define ENABLE_BT_PROTOCOL_TRACES to enable Bluetooth Protocol/Profile level traces
#GLOBAL_DEFINES     += ENABLE_BT_PROTOCOL_TRACES
#GLOBAL_DEFINES     += BT_TRACE_PROTOCOL=TRUE
#GLOBAL_DEFINES     += BT_USE_TRACES=TRUE
GLOBAL_DEFINES     += USE_MEM_POOL

ifneq (,$(findstring USE_MEM_POOL,$(GLOBAL_DEFINES)))
$(NAME)_SOURCES   += mem_pool/mem_pool.c
$(NAME)_INCLUDES  += ./mem_pool
#GLOBAL_DEFINES    += MEM_POOL_DEBUG
endif

# Default stack sizez of Hardware worker IO thread is 512 bytes.
# Since the keypress events are handled in this thread, this is not
# enough (might overflow when debug prints are enabled).
# Making it 1K gives enough room in stack when debug prints are enabled
# and for future code addition.
GLOBAL_DEFINES     += HARDWARE_IO_WORKER_THREAD_STACK_SIZE=1024

VALID_OSNS_COMBOS  := ThreadX-NetX_Duo

VALID_PLATFORMS    := BCM9WCD1AUDIO BCM943909* BCM943907*
INVALID_PLATFORMS  := BCM943909B0FCBU BCM943907AEVAL* BCM943907WCD2

