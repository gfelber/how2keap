#!/bin/sh

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
cd $SCRIPTPATH

mkdir -p ../rootfs
cd ../rootfs
cp ../share/rootfs.cpio.gz .
gunzip ./rootfs.cpio.gz
fakeroot cpio -idm < ./rootfs.cpio
rm rootfs.cpio
