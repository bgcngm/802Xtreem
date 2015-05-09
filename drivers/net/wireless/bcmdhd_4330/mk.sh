#!/bin/sh
make -C /opt/android/kernel/nvidia/kernel M=`pwd` ARCH=arm CROSS_COMPILE=/opt/android/system/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- $1
chmod 444 bcmdhd.ko 
