# how2keap

```
####################################################
#                                                  #
#    Tired of bloated heap implementations?        #
#          __                                      #
#         |  | __ ____ _____  ______               #
#         |  |/ // __ \\__  \ \____ \              #
#         |    <\  ___/ / __ \|  |_> >             #
#    use  |__|_ \\___  >____  /   __/              #
#              \/    \/     \/|__|                 #
#                                                  #
####################################################
```

flag is in /dev/sda

modify ./rootfs/init to improve debugging

## run examples
just replace pwn.c with the example you want to run (i.e. dirty\_cred.c)

## helper scripts:

+ scripts/decompress.sh 
  run this to extract the rootfs.cpio.gz into ./rootfs
  
+ scripts/compress.sh 
  recompress ./rootfs into rootfs.cpio.gz (i.e. after changes were made)

+ scripts/build.sh
  build the exploit (pwn.c), and add it to the root of the filesystem /pwn,
  if changes were made (autorun in start-qemu.sh and run-gdb.sh)
  
+ scripts/start-qemu.sh
  start qemu vm

+ scripts/run-gdb.sh
  run qemu and attach gdb to it (expects to be run in tmux session),
  uses scripts/gdbinit

## helpful links
+ bootlin: https://elixir.bootlin.com/linux/v6.6.22/source
