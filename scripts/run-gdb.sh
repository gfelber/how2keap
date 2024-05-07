#!/bin/sh

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
cd $SCRIPTPATH

if ./build.sh; then

  tmux split -v "gdb ../share/bzImage -x gdbinit"

  exec qemu-system-x86_64 \
    -s \
    -kernel ../share/bzImage  \
    -cpu qemu64,+smap,+smep \
    -m 1G \
    -initrd ../rootfs.cpio.gz  \
    -hda ../share/flag.txt \
    -append "rootwait root=/dev/vda console=tty1 console=ttyS0 nokaslr"   \
    -monitor /dev/null \
    -nographic \
    -no-reboot

fi
