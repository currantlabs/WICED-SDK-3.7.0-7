#!/bin/bash

#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

if [ "$PREREQUISITES_DONE" != "yes" ]; then
source ./prerequisites.sh
fi

###################################################################################################
#
# Download OpenOCD Tar ball
#
###################################################################################################

# Fetch OpenOCD - clone it then zip the cloned directory for later use
if [ ! -e ./$DOWNLOAD_DIR/openocd-$OPENOCD_REPO_HASH.tar.gz ]; then
    echo "Downloading OpenOCD"
    cd ./$DOWNLOAD_DIR

    # OpenOCD has some utf-8 filenames and MinGW cannot delete them.
    # Use the Windows command console to delete the files (does not delete the empty directories).
    if [ "$OSTYPE" == "msys" ]; then
        cmd /c "del /S /F /Q $OPENOCD_REPO_DIR"
    fi

    rm -rf ./$OPENOCD_REPO_DIR/

    git clone $OPENOCD_REPO_URL
    cd $OPENOCD_REPO_DIR
    git config --local user.email broadcom_wiced@broadcom.local
    git config --local user.name  "Broadcom Wiced"
    git checkout  $OPENOCD_REPO_HASH
    git submodule init
    git submodule update
    cd ..

    # Openocd has some nasty UTF-8 unicode filenames in the tcl scripts that create trouble in some msys environments
    # Consequently, only tar the .git directory then later do a "git reset" to recover files
    # --warning=no-file-changed is required due to corporate spyware interference on Windows platforms
    if [ "$OSTYPE" == "msys" ]; then
        tar --warning=no-file-changed -zcvf openocd-$OPENOCD_REPO_HASH.tar.gz $OPENOCD_REPO_DIR/.git
    else
        tar -zcvf openocd-$OPENOCD_REPO_HASH.tar.gz $OPENOCD_REPO_DIR/.git
    fi

    # OpenOCD has some utf-8 filenames and MinGW cannot delete them.
    # Use the Windows command console to delete the files (does not delete the empty directories).
    if [ "$OSTYPE" == "msys" ]; then
        cmd /c "del /S /F /Q $OPENOCD_REPO_DIR"
    fi

    rm -rf $OPENOCD_REPO_DIR/

    cd ..
fi


###################################################################################################
#
# Build OpenOCD
#
###################################################################################################


# Extract OpenOCD
echo "Extracting OpenOCD"
rm -rf ./$OPENOCD_REPO_DIR/
tar -zxvf ./$DOWNLOAD_DIR/openocd-$OPENOCD_REPO_HASH.tar.gz

# Checkout the OpenOCD code
echo "Checking out OpenOCD code"
cd $OPENOCD_REPO_DIR
git reset --hard
git checkout $OPENOCD_REPO_HASH
git branch -D Broadcom_only || true
git checkout -b Broadcom_only
#git submodule init
#git submodule update

# Patching OpenOCD
echo "Patching OpenOCD"

git am ../patches/0030-Added-support-for-STM32F412-device-programming.patch
git am ../patches/0034-Add-NuttX-RTOS-awareness.patch
git am ../patches/0035-Fix-Cortex-A-step-logic-when-polling-for-halt.patch

cd ..

if [ "$DEBUG_OPENOCD" == "yes" ]; then
    export EXTRA_CFLAGS=-"g -O0"
fi

if [ "$OSTYPE" == "msys" ]; then
    export EXTRA_OPENOCD_CFGOPTS="--enable-parport-giveio"
else
    export EXTRA_OPENOCD_CFGOPTS=""
    #Note: Using --enable-buspirate --enable-zy1000-master --enable-zy1000 causes ONLY the zy1000 adapter to be supported.
fi

#if [ "$OSTYPE" == "linux-gnu" ]; then
#    export EXTRA_OPENOCD_CFGOPTS="--enable-amtjtagaccel --enable-gw16012"
#fi

if [[ "$OSTYPE" == *darwin* ]]; then
#    export EXTRA_CFLAGS="$EXTRA_CFLAGS -framework IOKit -framework CoreFoundation"
#export EXTRA_CFLAGS="$EXTRA_CFLAGS -L`pwd`/libusb-install/lib/" # -L/opt/local/lib/" # -lusb-1.0"
    export EXTRA_CFLAGS="$EXTRA_CFLAGS -L`pwd`/libusb-compat-install/lib/ -Qunused-arguments"
else
    if [[ ! "$OSTYPE" == "msys2" ]]; then
        # These require linux/parport.h - hence do not work on OS-X
        export EXTRA_OPENOCD_CFGOPTS="$EXTRA_OPENOCD_CFGOPTS --enable-amtjtagaccel --enable-gw16012 --enable-parport "
    fi
    export EXTRA_CFLAGS="$EXTRA_CFLAGS -L`pwd`/libusb-win32-src-$LIBUSB_WIN32_VER/"
    export EXTRA_CFLAGS="$EXTRA_CFLAGS -Wl,--start-group "
fi

export CC_VAL="gcc"


# Build OpenOCD
echo "Building OpenOCD"
rm -rf openocd-build openocd-install
mkdir -p openocd-build
mkdir -p openocd-install
cd $OPENOCD_REPO_DIR
./bootstrap

cd jimtcl
git config --local user.email broadcom_wiced@broadcom.local
git config --local user.name  "Broadcom Wiced"
git reset --hard
git am ../../patches/JIM_0001-Add-variable-tracing-functionality.patch
git am ../../patches/JIM_0002-JimTCL-Define-S_IRWXG-and-S_IRWXO-for-platforms-win3.patch
cp ../../patches/msys2-config.guess autosetup/config.guess
cp ../../patches/msys2-config.sub   autosetup/config.sub
cd ../..

cd $OPENOCD_REPO_DIR/src/jtag/drivers/libjaylink/
git am ../../../../../patches/libjaylink-attributes-Fix-MSYS2-compilation-It-does-not-define-.patch
cd ../../../../..

cd openocd-build
export PKG_CONFIG_PATH="`pwd`/../hidapi-install/lib/pkgconfig:`pwd`/../libusb-install/lib/pkgconfig:`pwd`/../libusb-compat-install/lib/pkgconfig:`pwd`/../libftdi-install/lib/pkgconfig:`pwd`/../libusb-win32-src-$LIBUSB_WIN32_VER/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH=`pwd`/../libftdi-install/lib/:$LD_LIBRARY_PATH
../$OPENOCD_REPO_DIR/configure $EXTRA_OPENOCD_CFGOPTS \
                    --enable-dummy \
                    --enable-ftdi \
                    --enable-stlink \
                    --enable-ti-icdi \
                    --enable-ulink \
                    --enable-usb-blaster-2 \
                    --enable-vsllink \
                    --enable-jlink \
                    --enable-osbdm \
                    --enable-opendous \
                    --enable-aice \
                    --enable-usbprog \
                    --enable-rlink \
                    --enable-armjtagew \
                    --enable-cmsis-dap \
                    --enable-legacy-ft2232_libftdi \
                    --enable-jtag_vpi \
                    --enable-usb_blaster_libftdi \
                    --enable-ep93xx \
                    --enable-at91rm9200 \
                    --enable-bcm2835gpio \
                    --enable-presto_libftdi \
                    --enable-openjtag_ftdi \
                    --disable-option-checking  \
                    --prefix=`pwd`/../openocd-install/ \
                    --program-suffix=-all-brcm-libftdi \
                    --enable-maintainer-mode  \
                    PKG_CONFIG="pkg-config" \
                    CC="$CC_VAL" \
                    CFLAGS="$EXTRA_CFLAGS -g \
                        -I`pwd`/../libusb-compat-install/include/ \
                        -I`pwd`/../libusb-install/include/libusb-1.0 \
                        -L`pwd`/../libusb-compat-install/lib/ \
                        -L`pwd`/../libusb-install/lib/ \
                        -lusb-1.0 \
                        -L`pwd`/../libftdi-install/lib \
                        -I`pwd`/../libusb-win32-src-$LIBUSB_WIN32_VER/src/ \
                        -I/opt/local/include \
                        -I`pwd`/../$OPENOCD_REPO_DIR/src/jtag/drivers/hndjtag/include/ \
                        "

# These configure options require the FTDI ftd2xx library which
# is copyright FTDI with a non-compatible license
# --enable-legacy-ft2232_ftd2xx
# --enable-usb_blaster_ftd2xx
# --enable-presto_ftd2xx
# --enable-openjtag_ftd2xx

# not compatible with --enable-zy1000
# --enable-minidriver-dummy

# Fails to compile on mingw
# --enable-remote-bitbang
# --enable-sysfsgpio
# --enable-ioutil
# --enable-oocd_trace

make
make install
cd ..

# Copying OpenOCD into install directory
echo "Copying OpenOCD into install directory"
cp openocd-install/bin/openocd-all-brcm-libftdi* $INSTALL_DIR/$HOST_TYPE/

# Strip OpenOCD
if [ ! "$DEBUG_OPENOCD" == "yes" ]; then
    echo "Stripping executable"
    for f in \
        `find ./$INSTALL_DIR/$HOST_TYPE/ -name openocd-all-brcm-libftdi*`
    do
        strip $f
    done
fi

# OSX cannot be built static, so make a script to force it to find the dynamic libs
if [[ "$OSTYPE" == *darwin* ]]; then
    mv $INSTALL_DIR/$HOST_TYPE/openocd-all-brcm-libftdi $INSTALL_DIR/$HOST_TYPE/openocd-all-brcm-libftdi_run
    cp `which dirname` $INSTALL_DIR/$HOST_TYPE/openocd-all-brcm-libftdi_dirname

    echo "#!/bin/bash" > $INSTALL_DIR/$HOST_TYPE/openocd-all-brcm-libftdi
    echo "export DYLD_LIBRARY_PATH=\`\$0_dirname \$0\`:$DYLD_LIBRARY_PATH" >> $INSTALL_DIR/$HOST_TYPE/openocd-all-brcm-libftdi
    echo "\${0}_run \"\$@\"" >> $INSTALL_DIR/$HOST_TYPE/openocd-all-brcm-libftdi
    chmod a+x $INSTALL_DIR/$HOST_TYPE/openocd-all-brcm-libftdi
fi

echo
echo "Done! - Output is in $INSTALL_DIR"
echo

