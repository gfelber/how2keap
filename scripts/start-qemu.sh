#!/bin/bash

SCRIPTPATH="$( cd -- "$(dirname $(dirname "$0"))" >/dev/null 2>&1 ; pwd -P )"
cd $SCRIPTPATH

HELP=$(cat << EOM
$0 [OPTIONS]
-b build and compress rootfs if changed
-d build with -DDEBUG
-g run with GDB (kaslr still enabled)
-k disable kaslr
-c force compress rootfs
-h print this message
EOM
)
GDB=0
BUILD=0
COMPRESS=0
NOKASLR=0
DEBUG=0
while getopts "bdgkch" opt
do
    case $opt in
    (b) do_all=0 ; BUILD=1 ;;
    (d) do_all=0 ; DEBUG=1; BUILD=1 ;;
    (g) do_all=0 ; GDB=1;;
    (k) do_all=0 ; NOKASLR=1 ;;
    (c) do_all=0 ; COMPRESS=1 ;;
    (h) do_all=0 ; echo "$HELP"; exit 0 ;;
    (*) printf "Illegal option '-%s'\n" "$opt" && exit 1 ;;
    esac
done


if [ ! -d ./rootfs ]; then
  cp ./share/rootfs.cpio.gz ./
  ./scripts/decompress.sh
fi

if [ $BUILD -eq 1 ]; then
  DEBUG=$DEBUG ./scripts/build.sh || exit 1;
fi

if [ $COMPRESS -eq 1 ]; then
  ./scripts/compress.sh || exit 1;
fi


QARGS=""
KARGS="rootwait root=/dev/vda console=tty1 console=ttyS0"

if [ $GDB -eq 1 ]; then
  tmux split -v "gdb ./share/bzImage -x ./scripts/gdbinit"
  QARGS="-s"
fi

if [ $NOKASLR -eq 1 ]; then
  KARGS="$KARGS nokaslr"
fi

FLAG_FILE=$(mktemp)
printenv FLAG > $FLAG_FILE

exec qemu-system-x86_64 \
  $QARGS \
  -kernel ./share/bzImage  \
  -cpu qemu64,+smap,+smep \
  -m 512M \
  -smp 3 \
  -drive file=$FLAG_FILE,format=raw \
  -drive file=./out/pwn,format=raw \
  -initrd ./rootfs.cpio.gz  \
  -append "$KARGS" \
  -monitor /dev/null \
  -nographic

rm $FLAG_FILE
