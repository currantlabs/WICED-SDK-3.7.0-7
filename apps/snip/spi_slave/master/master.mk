#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME               := SPI_Master_Example_Application

$(NAME)_COMPONENTS := libraries/drivers/spi_slave/master

$(NAME)_SOURCES    := spi_master_app.c

VALID_PLATFORMS    := BCM943362WCD4 \
                      BCM958100SVK

# Temporarily used for BCM958100SVK
ifneq (,$(filter $(PLATFORM),BCM958100SVK))
GLOBAL_DEFINES += SPI_MASTER_NO_INT BCM958100SVK
endif

