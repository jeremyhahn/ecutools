#!/bin/bash

. ./vars.sh

if [[ -z "$RPIHOME" ]]; then
   error "RPIHOME is not set"
fi

if [[ -z "$KERNEL_BIN" ]]; then
   error "KERNEL_BIN is not set"
fi

if [[ -z "$MODULES_BIN" ]]; then
   error "KERNEL_BIN is not set"
fi

if [[ -z "$ROOTFS" ]]; then
   error "KERNEL_BIN is not set"
fi

rm -rf ${KERNEL_BIN}/*
rm -rf ${MODULES_BIN}/*
rm -rf ${ROOTFS}
rm -rf kernel-${VERSION}*
rm -rf linux-rpi-${VERSION}.y*
rm -rf rpi-${VERSION}.y.tar.gz
rm -rf can-modules
rm -rf can-modules.tar.gz
rm -rf modules/
rm -rf firmware
rm -rf tools
rm -rf spi-config
