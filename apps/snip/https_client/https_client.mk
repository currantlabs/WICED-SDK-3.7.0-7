#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_HTTPS_Client

$(NAME)_SOURCES    := https_client.c

$(NAME)_COMPONENTS := protocols/HTTP

# < Please add client certificate and private key here if you want to enable client authentication >
#CERTIFICATE := $(SOURCE_ROOT)resources/certificates/brcm_demo_server_cert.cer
#PRIVATE_KEY := $(SOURCE_ROOT)resources/certificates/brcm_demo_server_cert_key.key