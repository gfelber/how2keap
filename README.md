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

exploit is located inside the vm in /pwn (recommend running with `while ! /pwn; do test; done`)

## Techniques

| File                          | Technique                                                    | Linux-Version | Applicable CTF Challenges                             |
| ----------------------------- | ------------------------------------------------------------ | ------------- | ----------------------------------------------------- |
| [dirty\_cred.c](/dirty_cred.c) | [DirtyCred](https://github.com/Markakd/DirtyCred) abuses the heap memory reuse mechanism to get privileged | latest        | [Wall Rose](https://ctf2023.hitcon.org/dashboard/#15) |

## run examples
just replace pwn.c with the example you want to run (i.e. dirty\_cred.c)

## helper scripts:

+ scripts/start-qemu.sh [-g] [-b]  
 start qemu vm, -g with gdb (nokalsr), -b run ./build.sh first

+ scripts/decompress.sh   
  run this to extract the rootfs.cpio.gz into ./rootfs
 
+ scripts/compress.sh   
  recompress ./rootfs into rootfs.cpio.gz (i.e. after changes were made)

+ scripts/build.sh  
  build the exploit (pwn.c), and add it to the root of the filesystem /pwn

## buildroot
download [buildroot](https://buildroot.org/download.html) and extract
apply buildroot keap.patch using patch

```bash
patch -p1 -i buildroot/keap.patch -d ./PATH/TO/BUIDLROOT
```
now you can make changes using `make menuconfig` (e.g. changing kernel version) and recompile keap using `make` (might take a while)
the final files (rootfs.cpio.gz and bzImage) are located inside the buildroot dir inside `./output/images`

## helpful links
+ bootlin: https://elixir.bootlin.com/linux/v6.6.22/source

