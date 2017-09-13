#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_httpbin.org_Client

$(NAME)_SOURCES    := httpbin_org.c

$(NAME)_COMPONENTS := protocols/HTTP_client \
                      libraries/utilities/JSON_parser
