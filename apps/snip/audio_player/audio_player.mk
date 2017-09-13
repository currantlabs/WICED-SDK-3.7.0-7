#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_Audio_Player

#INCLUDE_MP3_DECODER	:= 1	# NOT working yet
#INCLUDE_ALAW_DECODER	:= 1	# NOT working yet

#WICED_ENABLE_TRACEX := 1
GLOBAL_DEFINES     += WICED_DISABLE_WATCHDOG
GLOBAL_DEFINES     += WICED_DISABLE_TLS
GLOBAL_DEFINES     += WICED_DISABLE_MCU_POWERSAVE
GLOBAL_DEFINES     += WPRINT_ENABLE_APP_DEBUG
GLOBAL_DEFINES     += WPRINT_ENABLE_LIB_DEBUG

# ENABLE for ethernet support
#$(NAME)_DEFINES   += MFG_TEST_ENABLE_ETHERNET_SUPPORT

APPLICATION_DCT    := audio_player_dct.c

GLOBAL_DEFINES     += APPLICATION_STACK_SIZE=12000

$(NAME)_SOURCES    := audio_player.c	\
					  audio_player_config.c		\
					  audio_player_dct.c		\
					  audio_player_http.c		\
					  audio_player_util.c		\
					  audio_player_flac.c		\
					  audio_player_wav.c


$(NAME)_COMPONENTS := audio/apollo/audio_render \
                      audio/apollo/apollocore \
					  audio/codec/FLAC \
                      utilities/command_console \
                      utilities/command_console/wifi \
                      filesystems/wicedfs \
					  protocols/DNS \
					  protocols/HTTP

# MP3 specific files & libs
ifeq (1,$(INCLUDE_MP3_DECODER))
$(NAME)_SOURCES    += wiced_mp3_interface.c
$(NAME)_COMPONENTS += audio/codec/MP3
endif

# aLaw uLaw specific files & libs
ifeq (1,$(INCLUDE_ALAW_DECODER))
$(NAME)_SOURCES    += wiced_aLawuLaw_interface.c
$(NAME)_COMPONENTS += audio/codec/aLaw_uLaw
endif


#GLOBAL_DEFINES     += CONSOLE_ENABLE_WL
ifneq (,$(findstring CONSOLE_ENABLE_WL,$(GLOBAL_DEFINES)))
# wl commands which dump a lot of data require big buffers.
GLOBAL_DEFINES   += WICED_PAYLOAD_MTU=8320
$(NAME)_COMPONENTS += test/wl_tool
endif

WIFI_CONFIG_DCT_H  := wifi_config_dct.h

ifdef WICED_ENABLE_TRACEX
$(info apollo_audio using tracex lib)
GLOBAL_DEFINES     += WICED_TRACEX_BUFFER_DDR_OFFSET=0x0
GLOBAL_DEFINES     += WICED_TRACEX_BUFFER_SIZE=0x200000
$(NAME)_COMPONENTS += test/TraceX
endif

GLOBAL_DEFINES     += RX_PACKET_POOL_SIZE=10


GLOBAL_DEFINES     += WICED_USE_AUDIO


VALID_OSNS_COMBOS  := ThreadX-NetX_Duo
VALID_PLATFORMS    := BCM943909WCD* BCM943907*
INVALID_PLATFORMS  := BCM943907AEVAL*
