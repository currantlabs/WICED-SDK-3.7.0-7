#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_OTA_Server

$(NAME)_SOURCES := wiced_ota2_server.c \
                   ota2_server_web_page.c \
                   ota2_server.c
GLOBAL_INCLUDES := .

$(NAME)_COMPONENTS := filesystems/ota2
