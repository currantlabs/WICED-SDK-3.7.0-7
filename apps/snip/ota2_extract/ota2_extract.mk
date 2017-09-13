#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_OTA2_extract

$(NAME)_SOURCES := ota2_extract.c

$(NAME)_COMPONENTS := daemons/ota2_server			\
                      daemons/ota2_service		 	 \
                      utilities/mini_printf 		 \
                      filesystems/wicedfs			 \
					  protocols/DNS 				 \
					  protocols/HTTP

VALID_OSNS_COMBOS  := ThreadX-NetX ThreadX-NetX_Duo

VALID_PLATFORMS    := BCM943909WCD1_3.B0 BCM943909WCD1_3.B1 BCM943907WAE_1.B0 BCM943907WAE_1.B1
VALID_PLATFORMS    += BCM943907AEVAL2F_1.B0 BCM943907AEVAL2F_1.B1 BCM943907AEVAL1F_1.B0 BCM943907AEVAL1F_1.B1 BCM943907WAE2_1.B1
VALID_PLATFORMS    += BCM943907WCD1
INVALID_PLATFORMS	:= BCM943909WCD1_3_SDIO