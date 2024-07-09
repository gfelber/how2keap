#!/bin/sh

SCRIPTPATH="$( cd -- "$(dirname $(dirname "$0"))" >/dev/null 2>&1 ; pwd -P )"
cd $SCRIPTPATH

cd ./rootfs
find . -print0 \
| fakeroot cpio --null -ov --format=newc \
| gzip -9 > ../rootfs.cpio.gz
exit
