function error {
    echo $1
    exit 1
}

RPIHOME=/storage/sources/raspberrypi
VERSION=3.10
THREADS=8
ROOTFS=${RPIHOME}/rootfs
PLATFORM=bcmrpi
RPICONF=${RPIHOME}/rpi-${VERSION}-can-pps.config

#export CFLAGS="-pipe -Ofast -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp -fomit-frame-pointer -fexcess-precision=fast -flto -fuse-linker-plugin"
#export CPPFLAGS="-pipe -Ofast -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp -fomit-frame-pointer -fexcess-precision=fast -flto -fuse-linker-plugin"
#export CCPREFIX=${RPIHOME}/tools/arm-bcm2708/arm-bcm2708hardfp-linux-gnueabi/bin/arm-bcm2708hardfp-linux-gnueabi-
#export CCPREFIX=/usr/bin/arm-linux-gnueabihf-

export CCPREFIX=/usr/bin/arm-linux-gnueabi-
export KERNEL_SRC=${RPIHOME}/linux-rpi-${VERSION}.y
export KERNEL_BIN=${RPIHOME}/kernel-${VERSION}
export MODULES_BIN=${RPIHOME}/modules
