#!/bin/bash

DEFCONFIG_FILE=m7cdug_defconfig

if [ -z "$DEFCONFIG_FILE" ]; then
	echo "Need defconfig file(j1v-perf_defconfig)!"
	exit -1
fi

if [ ! -e arch/arm/configs/$DEFCONFIG_FILE ]; then
	echo "No such file : arch/arm/configs/$DEFCONFIG_FILE"
	exit -1
fi


# define toolchain
CC=${HOME}/android/cm-11.0/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-

# create .config
if [ ! -e ./.config ]; then
	echo "**** Creating .config file ****"
	env KCONFIG_NOTIMESTAMP=true \
	make ${DEFCONFIG_FILE}
fi

# build the kernel
echo "**** Building kernel ****"
make -j3 ARCH=arm SUBARCH=arm CROSS_COMPILE=$CC
if [ -e ./arch/arm/boot/zImage ]; then
	echo "**** Successfully built kernel ****"

	echo "**** Copying kernel to build_result/kernel/ ****"
	mkdir -p ./build_result/kernel/
	cp ./arch/arm/boot/zImage ./build_result/kernel/zImage

	echo "**** Copying all built modules (.ko) to build_result/modules/ ****"
	mkdir -p ./build_result/modules/
	for file in $(find ./ -name *.ko); do
		cp $file ./build_result/modules/
	done
fi

