#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := ipv6_multicast

$(NAME)_SOURCES    := ipv6_multicast.c

VALID_OSNS_COMBOS  := ThreadX-NetX_Duo ThreadX-NetX
WIFI_CONFIG_DCT_H  := wifi_config_dct.h

GLOBAL_DEFINES += AUTO_IP_ENABLED