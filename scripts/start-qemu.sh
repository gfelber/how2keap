#!/bin/bash

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
cd $SCRIPTPATH

HELP="$0 [-b] [-g]\n\t-g run with GDB (also disables kaslr)\n\t-b BUILD and compress rootfs if changed\n"
GDB=0
BUILD=0
COMPRESS=0
while getopts "gbhc" opt
do
    case $opt in
    (g) do_all=0 ; GDB=1 ;;
    (b) do_all=0 ; BUILD=1 ;;
    (c) do_all=0 ; COMPRESS=1 ;;
    (h) do_all=0 ; printf $HELP && exit 0 ;;
    (*) printf "Illegal option '-%s'\n" "$opt" && exit 1 ;;
    esac
done

if [ $BUILD -eq 1 ]; then
  ./build.sh || exit 1;
fi

if [ $COMPRESS -eq 1 ]; then
  ./compress.sh || exit 1;
fi


QARGS=""
KARGS="rootwait root=/dev/vda console=tty1 console=ttyS0"

if [ $GDB -eq 1 ]; then
  tmux split -v "gdb ../share/bzImage -x gdbinit"
  QARGS="-s"
  KARGS="$KARGS nokaslr"
fi

exec qemu-system-x86_64 \
  $QARGS \
  -kernel ../share/bzImage  \
  -cpu qemu64,+smap,+smep \
  -m 512M \
  -smp 3 \
  -drive file=../share/flag.txt,format=raw \
  -initrd ../rootfs.cpio.gz  \
  -append "$KARGS" \
  -monitor /dev/null \
  -nographic
