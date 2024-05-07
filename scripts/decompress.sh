#!/bin/sh

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
cd $SCRIPTPATH

if [ ! -f ../share/bzImage ] || [ ! -f ../share/rootfs.cpio.gz ] ; then
  URL=$(curl -s https://api.github.com/repos/gfelber/how2keap/releases/latest | jq '.assets[0].browser_download_url' -r);
  wget $URL -O /tmp/keap.tar.gz;
  tar -xvf /tmp/keap.tar.gz -C ../;
fi

mkdir -p ../rootfs
cd ../rootfs
cp ../share/rootfs.cpio.gz .
gunzip ./rootfs.cpio.gz
fakeroot cpio -idm < ./rootfs.cpio
rm rootfs.cpio
