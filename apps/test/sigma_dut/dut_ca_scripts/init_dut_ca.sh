#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

echo USB ports before initialising FTDI
sudo ls -l /dev/ttyUSB*
sudo ifconfig eth0 down
sudo ifconfig eth0 192.168.250.40 netmask 255.255.0.0 up
sudo rmmod ftdi_sio
sudo modprobe ftdi_sio vendor=0xa5c product=0x43fa
dmesg | grep FTDI | grep attach
#echo USB ports after initialising FTDI
#sudo ls -l /dev/ttyUSB*

