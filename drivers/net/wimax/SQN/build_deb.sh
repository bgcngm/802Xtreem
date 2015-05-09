#!/bin/bash

### Build deb package for USB driver


#set -x


usage () {
	echo "usage: $0 ubuntu|debian";
}

sqn_error () {
	echo "Error";
	exit 1;
}


if ! [[ "$1" = "ubuntu" || "$1" = "debian" ]]; then
	usage;
	exit 1;
fi


DISTRO="$1"
DEBIAN_DIR="DEBIAN_$DISTRO"

DEB_FILE_LIST="$DEBIAN_DIR/control $DEBIAN_DIR/postinst $DEBIAN_DIR/prerm"
DRIVER_SRC_FILE_LIST="Makefile msg.h thp.c thp.h usb-driver.c version.h update_modules_order.pl"
CHECK_FILE_LIST="$DEB_FILE_LIST $DRIVER_SRC_FILE_LIST dkms.conf 99-sequans.rules"
DRIVER_VERSION=$(grep 'SQN_MODULE_VERSION' version.h | sed -r 's/.*"(.*)"/\1/')
PKG_DIR="sequans-driver_$DRIVER_VERSION"
PKG_SRC_DIR="$PKG_DIR/usr/src/sequans-$DRIVER_VERSION"
PKG_UDEV_DIR="$PKG_DIR/etc/udev/rules.d"
PKG_NAME="$PKG_DIR-$DISTRO.deb"
VERSION_PLACEHOLDER=sqn_driver_version_placeholder


if ! ls $CHECK_FILE_LIST > /dev/null 2>&1; then
	echo "Some of the files needed for build are missing, check \
that you run this script from driver's src directory and all of the \
following files are present:"
	echo -e "\n$CHECK_FILE_LIST"
	exit 1
fi

if [[ -d $PKG_DIR ]]; then
	rm -rf $PKG_DIR || sqn_error
fi

if [[ -f $PKG_NAME ]]; then
	rm -f $PKG_NAME || sqn_error
fi

mkdir -p $PKG_SRC_DIR || sqn_error
cp $DRIVER_SRC_FILE_LIST $PKG_SRC_DIR || sqn_error
sed "s/$VERSION_PLACEHOLDER/$DRIVER_VERSION/" dkms.conf > $PKG_SRC_DIR/dkms.conf \
	|| sqn_error
mkdir -p $PKG_UDEV_DIR || sqn_error
cp 99-sequans.rules $PKG_UDEV_DIR || sqn_error
cp -a $DEBIAN_DIR $PKG_DIR/DEBIAN || sqn_error
rm -rf $PKG_DIR/DEBIAN/.svn || sqn_error

for i in $DEB_FILE_LIST; do
	sed -i "s/$VERSION_PLACEHOLDER/$DRIVER_VERSION/" "$PKG_DIR/DEBIAN/$(basename $i)" \
	|| sqn_error
done

dpkg-deb -b $PKG_DIR $PKG_NAME || sqn_error
rm -rf $PKG_DIR || sqn_error

echo "Done"
