#!/bin/bash

# Download and compile rpi kernel 3.10.y, latest firmware, CAN drivers, and apply latest patches

sudo apt-get update
sudo apt-get -y install kernel-package

#http://elinux.org/RPi_Kernel_Compilation
#http://www.satsignal.eu/raspberry-pi/kernel-cross-compile.html
#http://lnxpps.de/rpie/raspi-anleitung.txt

# Include variables
. ./vars.sh

# Ensure clean start
. ./clean.sh
mkdir ${KERNEL_BIN}
mkdir ${MODULES_BIN}
mkdir ${ROOTFS}

wget https://github.com/raspberrypi/linux/archive/rpi-${VERSION}.y.tar.gz
[ $? = "0" ] || error "Error downloading Kernel source"
tar -xzf rpi-${VERSION}.y.tar.gz
[ $? = "0" ] || error "Error extracting Kernel source"

# Compile kernel
cd $KERNEL_SRC
make mrproper
[ $? = "0" ] || error "Error running mrproper"

if [ -f ${RPICONF} ];
then
      cp ${RPICONF} ${KERNEL_BIN}/.config

      make O=${KERNEL_BIN} ARCH=arm CROSS_COMPILE=${CCPREFIX} oldconfig -j${THREADS}
      [ $? = "0" ] || error "Error generating default kernel using oldconfig"
else
      make O=${KERNEL_BIN} ARCH=arm CROSS_COMPILE=${CCPREFIX} ${PLATFORM}_defconfig
      [ $? = "0" ] || error "Error generating default kernel configuration"

      make O=${KERNEL_BIN} ARCH=arm CROSS_COMPILE=${CCPREFIX} oldconfig -j${THREADS}
      [ $? = "0" ] || error "Error generating default kernel using oldconfig"

      make O=${KERNEL_BIN} ARCH=arm CROSS_COMPILE={$CCPREFIX} menuconfig -j${THREADS}
      [ $? = "0" ] || error "Error running make menuconfig"

	#General setup --->
	#  Cross-compiler tool prefix (/usr/bin/arm-linux-gnueabi-)
	#  Default hostname (raspberrypi)
	#  Timers subsystem --->
	#    Timer tick handling --->
	#      Periodic timer ticks: checked

	#[*] Networking support --->
	#....<M> CAN bus subsystem support --->
	#........<M> Raw CAN Protocol (raw access with CAN-ID filtering)
	#........<M> Broadcast Manager CAN Protocol (with content filtering)
	#............CAN Device Drivers --->
	#................<M> Platform CAN drivers with Netlink support
	#................[*] CAN bit-timing calculation
	#................<M> Microchip MCP251x SPI CAN controllers
	#................[*] CAN devices debugging messages

	#Device Drivers --->
	#....[*] SPI support --->
	#............<M> BCM2798 SPI controller driver (SPI0)
	#............<M> User mode SPI driver support
	#....PPS support --->
	#............<M> PPS support
	#................*** PPS clients support
	#............<M> PPS client using GPIO
	#.-*-GPIO Support --->
	#............[*] /sys/class/gpio/... (sysfs interface)
	#............[*] Generic memory-mapped GPIO controller support

      cp ${KERNEL_BIN}/.config ${RPICONF}
fi

make O=${KERNEL_BIN} ARCH=arm CROSS_COMPILE=${CCPREFIX} -j${THREADS}
[ $? = "0" ] || error "Error compiling kernel"

make O=${KERNEL_BIN} ARCH=arm CROSS_COMPILE=${CCPREFIX} modules -j${THREADS}
[ $? = "0" ] || error "Error compiling modules"

# Compile latest BCM2708 SPI driver
wget https://raw.github.com/notro/spi-bcm2708/master/spi-bcm2708.c
wget  https://raw.github.com/lampeh/rpi-misc/master/linux-pps/linux-rpi-pps-gpio-bcm2708.diff
patch arch/arm/mach-bcm2708/bcm2708.c < linux-rpi-pps-gpio-bcm2708.diff
patch arch/arm/mach-bcm2708/bcm2708.c < ../bcm2708_mcp251x_3.10.patch
[ $? = "0" ] || error "Error patching bcm2708"

# Install kernel modules
cd $KERNEL_SRC
make O=${KERNEL_BIN} ARCH=arm CROSS_COMPILE=${CCPREFIX} INSTALL_MOD_PATH=${MODULES_BIN} modules_install -j${THREADS}
[ $? = "0" ] || error "Error installing kernel modules"

# Compile spi-config
cd ${RPIHOME}
rm -rf spi-config
git clone https://github.com/msperl/spi-config
cd spi-config
make O=${KERNEL_BIN} ARCH=arm CROSS_COMPILE=${CCPREFIX} KDIR=$KERNEL_SRC -j${THREADS}
[ $? = "0" ] || error "Error building spi-config"
make O=${KERNEL_BIN} ARCH=arm CROSS_COMPILE=${CCPREFIX} INSTALL_MOD_PATH=${MODULES_BIN} KDIR=$KERNEL_SRC install -j${THREADS}
[ $? = "0" ] || error "Error installing spi-config"
cd ${RPIHOME}

# Compile firmware
git clone --depth 1 git://github.com/raspberrypi/firmware.git
cd firmware
[ $? = "0" ] || error "Error checking out latest firmware"
git fetch --depth 1 git://github.com/raspberrypi/firmware.git next:refs/remotes/origin/next
#git checkout next

# Create Raspberry PI build directory
rm -rf ${ROOTFS}
mkdir -p ${ROOTFS}/boot ${ROOTFS}/opt ${ROOTFS}/etc/modprobe.d ${ROOTFS}/etc/network
cp boot/bootcode.bin boot/fixup.dat boot/start.elf ${ROOTFS}/boot
cp ${RPIHOME}/modules.template ${ROOTFS}/etc/modules
cp ${RPIHOME}/raspi-blacklist.conf.template ${ROOTFS}/etc/modprobe.d/raspi-blacklist.conf
cp ${RPIHOME}/interfaces.template ${ROOTFS}/etc/network/interfaces
[ $? = "0" ] || error "Error copying firmware to rootfs/boot"
#Next, you need to copy the VC libraries over. There are two copies of this: one for hard float and one for soft float. To find the correct one, run the following command:
${CCPREFIX}gcc -v 2>&1 | grep hard
#If something prints out, and you can see --with-float=hard, you need the hard float ones. NOTE: The current version of Raspbian uses hard float.
cp -R opt/vc ${ROOTFS}/opt
cp ${KERNEL_BIN}/arch/arm/boot/zImage ${ROOTFS}/boot/kernel.img
[ $? = "0" ] || error "Error copying kernel to rootfs/boot"
cp -R ${MODULES_BIN}/lib ${ROOTFS}
cd ${RPIHOME}

# Make distributable archive of CAN related modules
CANSCRIPT=can-modules/init_canbus.sh
rm -rf can-modules
mkdir can-modules
cp ${KERNEL_BIN}/drivers/spi/spi-bcm2708.ko can-modules
cp ${KERNEL_BIN}/net/can/can.ko can-modules
cp ${KERNEL_BIN}/drivers/net/can/can-dev.ko can-modules
cp ${KERNEL_BIN}/net/can/can-raw.ko can-modules
cp ${KERNEL_BIN}/net/can/can-bcm.ko can-modules
cp ${KERNEL_BIN}/drivers/net/can/mcp251x.ko can-modules
touch ${CANSCRIPT}
echo "#!/bin/bash" > ${CANSCRIPT}
echo "#insmod spi-bcm2708.ko" >> ${CANSCRIPT}
echo "insmod can.ko" >> ${CANSCRIPT}
echo "insmod can-dev.ko" >> ${CANSCRIPT}
echo "insmod can-raw.ko" >> ${CANSCRIPT}
echo "insmod can-bcm.ko" >> ${CANSCRIPT}
echo "insmod mcp251x.ko" >> ${CANSCRIPT}
echo "ip link set can0 up type can bitrate 1000000" >> ${CANSCRIPT}
chmod +x ${CANSCRIPT}
tar czf can-modules.tar.gz can-modules
[ $? = "0" ] || error "Error creating can-modules archive"

echo "Build complete. Copy rootfs/* to the root of the Raspberry PI"

