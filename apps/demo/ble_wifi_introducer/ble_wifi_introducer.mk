#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Bluetooth_Low_Energy_WiFi_Introducer_Sensor_Application

$(NAME)_SOURCES    := ble_wifi_introducer.c \
                      wiced_bt_cfg.c

$(NAME)_INCLUDES   := .

$(NAME)_COMPONENTS := libraries/drivers/bluetooth/low_energy \
                      utilities/command_console \
                      utilities/command_console/wifi

$(NAME)_COMPONENTS += inputs/button_manager

WIFI_CONFIG_DCT_H  := wifi_config_dct.h

GLOBAL_DEFINES     += BUILDCFG

VALID_PLATFORMS += BCM9WCDPLUS114 \
                   BCM943909WCD* \
                   BCM943907* \
                   BCM943341WCD1 \
                   BCM9WCD1AUDIO \
                   BCM943438WLPTH_2

INVALID_PLATFORMS  += BCM943907WCD2 BCM943907AEVAL*