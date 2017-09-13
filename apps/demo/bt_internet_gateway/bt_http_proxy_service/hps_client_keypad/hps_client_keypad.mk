#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_Bluetooth_HPS_Enabled_Keypad


$(NAME)_SOURCES    := hps_client_keypad.c \
                      wiced_bt_config.c

$(NAME)_INCLUDES   := .

$(NAME)_COMPONENTS := drivers/bluetooth/low_energy

GLOBAL_DEFINES     := BUILDCFG

APPLICATION_DCT    := hps_client_keypad_dct.c

VALID_PLATFORMS    := BCM9WCDPLUS114 \
                      BCM943341WCD1 \
                      BCM943909WCD* \
                      BCM943438WCD1 \
                      BCM94343WWCDA_ext

INVALID_PLATFORMS  += BCM943907WCD2