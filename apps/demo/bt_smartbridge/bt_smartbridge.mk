#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_BT_SmartBridge

$(NAME)_SOURCES    := bt_smartbridge.c

$(NAME)_COMPONENTS += daemons/HTTP_server \
                      daemons/device_configuration \
                      protocols/SNTP \
                      daemons/Gedday \
                      drivers/bluetooth/low_energy \
                      daemons/bt_smartbridge

$(NAME)_RESOURCES  += apps/bt_smartbridge/smartbridge_report.html \
                      apps/bt_smartbridge/data.html \
                      images/brcmlogo.png \
                      images/brcmlogo_line.png \
                      images/favicon.ico \
                      styles/buttons.css \
                      scripts/general_ajax_script.js \
                      scripts/wpad.dat

GLOBAL_DEFINES     := USE_SELF_SIGNED_TLS_CERT

# enable bluetooth traces/logging
#GLOBAL_DEFINES     += ENABLE_BT_PROTOCOL_TRACES

APPLICATION_DCT    := bt_smartbridge_dct.c

VALID_PLATFORMS    := BCM9WCDPLUS114 \
                      BCM943341WCD1 \
                      BCM9WCD1AUDIO \
                      BCM943438WLPTH_2 \
                      BCM943909WCD1 \
                      BCM943909WCD1_3 \
                      BCM943438WCD1

INVALID_PLATFORMS  += BCM943907WCD2
