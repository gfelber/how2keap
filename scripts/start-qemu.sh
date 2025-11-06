#!/bin/bash

SCRIPTPATH="$( cd -- "$(dirname $(dirname "$0"))" >/dev/null 2>&1 ; pwd -P )"
cd $SCRIPTPATH

HELP=$(cat << EOM
$0 [OPTIONS]
-b build and compress rootfs if changed
-d build with -DDEBUG
-p run /pwn on startup
-g run with GDB (kaslr still enabled)
-k disable kaslr
-c force compress rootfs
-l set loglevel to 9
-r run as root
-n run with nsjail
-v run with kvm (host cpu)
-h print this message
EOM
)
BUILD=0
DEBUG=0
PWN=0
GDB=0
NOKASLR=0
COMPRESS=0
LOG=0
ROOT=0
NSJAIL=0
KVM=0
while getopts "bdpgkclrnvh" opt
do
    case $opt in
    (b) do_all=0 ; BUILD=1 ;;
    (d) do_all=0 ; DEBUG=1; BUILD=1 ;;
    (p) do_all=0 ; PWN=1 ;;
    (g) do_all=0 ; GDB=1;;
    (k) do_all=0 ; NOKASLR=1 ;;
    (c) do_all=0 ; COMPRESS=1 ;;
    (r) do_all=0 ; ROOT=1 ;;
    (l) do_all=0 ; LOG=1 ;;
    (n) do_all=0 ; NSJAIL=1 ;;
    (v) do_all=0 ; KVM=1 ;;
    (h) do_all=0 ; echo "$HELP"; exit 0 ;;
    (*) printf "Illegal option '-%s'\n" "$opt" && exit 1 ;;
    esac
done

if [ ! -f ./share/bzImage.unpack ]; then
  ./scripts/extract-image.sh ./share/bzImage > ./share/bzImage.unpack
fi

if [ ! -d ./rootfs ]; then
  cp ./share/rootfs.cpio.gz ./
  ./scripts/decompress.sh
  ./scripts/compress.sh || exit 1;
fi

if [ $BUILD -eq 1 ]; then
  DEBUG=$DEBUG ./scripts/build.sh || exit 1;
fi

if [ $COMPRESS -eq 1 ]; then
  ./scripts/compress.sh || exit 1;
fi


QARGS=""
KARGS="console=ttyS0"

if [ $PWN -eq 1 ]; then
  KARGS="$KARGS pwn"
fi

if [ $GDB -eq 1 ]; then
  addr=$(readelf -s out/pwn | awk '/kbreak/ {print $2; exit}')
  if [ -z "$addr" ]; then
    sed -i "/# kbreak/{n;s/.*/# hb * kbreak/}" ./scripts/gdbinit
  else
    sed -i "/# kbreak/{n;s/.*/hb * 0x$addr/}" ./scripts/gdbinit
  fi
  tmux split -v "gdb ./share/bzImage.unpack -x ./scripts/gdbinit"
  QARGS="-s"
fi


if [ $NOKASLR -eq 1 ]; then
  KARGS="$KARGS nokaslr"
fi

if [ $LOG -eq 1 ]; then
  KARGS="$KARGS loglevel=9"
else 
  KARGS="$KARGS loglevel=4"
fi

if [ $ROOT -eq 1 ]; then
  KARGS="$KARGS init=/bin/sh"
fi

if [ $NSJAIL -eq 1 ]; then
  KARGS="$KARGS nsjail"
fi

if test -z "$FLAG"; then
  FLAG="keap{ex4mpl3_fl4g}"
fi

CPU="-cpu qemu64,+rdrand,+smap,+smep"

if [ $KVM -eq 1 ]; then
  CPU="-cpu host --enable-kvm"
fi

FLAG_FILE=$(mktemp)
echo "$FLAG" > $FLAG_FILE

qemu-system-x86_64 \
  $QARGS \
  -kernel ./share/bzImage \
  $CPU \
  -m 512M \
  -smp 3 \
  -drive file=$FLAG_FILE,format=raw \
  -drive file=./out/pwn,format=raw \
  -initrd ./rootfs.cpio.gz  \
  -append "$KARGS" \
  -nographic

rm $FLAG_FILE
reset
