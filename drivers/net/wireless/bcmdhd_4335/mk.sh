#!/bin/sh
make -C /home/gatien/fw/kernel/1115/kernel/ M=`pwd` ARCH=arm CROSS_COMPILE=/home/gatien/fw/kernel/arm-eabi-4.4.3/bin/arm-eabi- $1
chmod 444 bcmdhd.ko
adb remount
adb push bcmdhd.ko /system/lib/modules/bcmdhd.ko 
adb reboot
