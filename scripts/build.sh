#!/bin/sh

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
cd $SCRIPTPATH

if [ ! -d ../rootfs ]; then
    ./decompress.sh
fi

MD5=$(md5sum ../rootfs/pwn)

cd ..
# gcc -static -pthread -no-pie  -Os ../pwn.c -o ../rootfs/pwn
make clean
make rootfs 
RESULT=$?
if [ $RESULT -ne 0 ]; then
  echo compile failed
  exit 1
fi

cd $SCRIPTPATH
if [ "$MD5" != "$(md5sum ../rootfs/pwn)" ]; then
  ./compress.sh
fi

