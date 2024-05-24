#!/bin/sh

FLAG_FILE=$(mktemp)

printenv FLAG > $FLAG_FILE

exec qemu-system-x86_64 \
  -kernel /home/keap/bzImage  \
  -cpu kvm64,+smap,+smep \
  -m 1G \
  -smp 2 \
  -initrd /home/keap/rootfs.cpio.gz  \
  -hda $FLAG_FILE \
  -append "rootwait root=/dev/vda console=tty1 console=ttyS0"   \
  -monitor /dev/null \
  -nographic \
  -no-reboot
