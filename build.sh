#!/bin/bash

clear
# allow user to choose which variant to build
PS3='Choose variant to build: '
select variant in "802W" "802D" "802T"
do
	if [ -z $variant ] ; then
		echo "Wrong selection!"
		exit -1
	else
		if [ $variant = "802W" ] ; then
			CODENAME=m7cdug
		else
			if [ $variant = "802D" ] ; then
				CODENAME=m7cdwg
			else
				CODENAME=m7cdtu
			fi
		fi
	fi
	break
done
clear
DEFCONFIG_FILE=$CODENAME"_defconfig"

if [ ! -e arch/arm/configs/$DEFCONFIG_FILE ]; then
	echo "No such file : arch/arm/configs/$DEFCONFIG_FILE"
	exit -1
fi

echo "**** $variant variant ****"


# define toolchain
CC=${HOME}/android/toolchains/arm-cortex_a15-linux-gnueabihf-linaro_4.8.3-2014.04/bin/arm-cortex_a15-linux-gnueabihf-

# create .config
if [ ! -e ./.config ]; then
	echo "**** Creating .config file ****"
	env KCONFIG_NOTIMESTAMP=true \
	make ${DEFCONFIG_FILE}
else
	grep -q $CODENAME ./.config
	if [ $? -ne 0 ]; then
		echo "**** Discarding old .config file ****"
		rm ./.config
		echo "**** Creating .config file ****"
		env KCONFIG_NOTIMESTAMP=true \
		make ${DEFCONFIG_FILE}
	fi
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

