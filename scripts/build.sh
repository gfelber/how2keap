#!/bin/bash

SCRIPTPATH="$( cd -- "$(dirname $(dirname "$0"))" >/dev/null 2>&1 ; pwd -P )"
cd $SCRIPTPATH

if [ ! -d ./rootfs ]; then
    ./scripts/decompress.sh
fi

MD5=$(md5sum ./rootfs/pwn)

make clean

if [ $DEBUG -eq 1 ]; then
  make rootfs CFLAGS=-DDEBUG
else
  make rootfs 
fi


RESULT=$?
if [ $RESULT -ne 0 ]; then
  echo compile failed
  exit 1
fi

if [ "$MD5" != "$(md5sum ./rootfs/pwn)" ]; then
  ./scripts/compress.sh
fi

