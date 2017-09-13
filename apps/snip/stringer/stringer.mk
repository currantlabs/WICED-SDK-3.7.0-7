#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

include ../console/stringer_pal.mk

NAME := App_Uart_Test

$(NAME)_SOURCES := $(STRINGER_SOURCES)

GLOBAL_DEFINES := WICED_DISABLE_STDIO

