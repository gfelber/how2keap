# how2keap

A WIP cheat sheet for various linux kernel heap exploitation techniques (and privilige escalations).

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

exploit is located inside the vm in /pwn (recommend running with `while ! /pwn; do true; done`)

## Techniques

### Privilige Escalation

| File                          | Technique                                                    | Linux-Version | Applicable CTF Challenges                             |
| ----------------------------- | ------------------------------------------------------------ | ------------- | ----------------------------------------------------- |
| [dirty\_cred.c](/linux6.6.22/dirty_cred.c) | [DirtyCred](https://github.com/Markakd/DirtyCred) abuses the heap memory reuse mechanism to get privileged | latest        | [Wall Rose](https://ctf2023.hitcon.org/dashboard/#15) |
| [dirty\_pagetable.c](/linux6.6.22/dirty_pagetable.c) | [Dirty Pagetable](https://yanglingxi1993.github.io/dirty_pagetable/dirty_pagetable.html) abuse pagetables to get unprotected AAR/AAW in kernel space (kernel RCE) | latest        | [keasy](https://ptr-yudai.hatenablog.com/entry/2023/12/08/093606#Dirty-Pagetable) |
| [dirty\_pagetable\_mp.c](/linux6.6.22/dirty_pagetable_mp.c) | [Dirty Pagetable](https://yanglingxi1993.github.io/dirty_pagetable/dirty_pagetable.html) abuse pagetables to get unprotected AAR/AAW in kernel space (modprobe) | latest        | [Faulty Kernel](https://github.com/DownUnderCTF/Challenges_2024_Public/tree/main/pwn/faulty-kernel) |
| [flag\_corrupt.c](/linux6.6.22/flag_corrupt.c) | use a UAF to corrupt /etc/passwd flags and get privileged | latest        | [Faulty Kernel](https://github.com/DownUnderCTF/Challenges_2024_Public/tree/main/pwn/faulty-kernel) |

### Gadgets
| File                          | Technique                                                    | Linux-Version | Applicable CTF Challenges                             |
| ----------------------------- | ------------------------------------------------------------ | ------------- | ----------------------------------------------------- |
| [cross\_cache.c](/linux6.6.22/cross_cache.c) | showcasing a cross cacheq attack that allows using dangeling ptrs to target heap of other slabs | latest  | [Wall Rose](https://ctf2023.hitcon.org/dashboard/#15)
| [slubstick.c](/linux6.6.22/slubstick.c) | [SLUBStick](https://github.com/IAIK/SLUBStick) more reliable way to trigger cross-cache  | latest        |  |
| [per\_cpu\_slabs.c](/linux6.6.22/per_cpu_slabs.c) | showcasing how slabs are managed and reallocated on a per cpu basis| latest  |
| [mmaped\_files.c](/linux6.6.22/mmaped_files.c) |   using mmaped files to create race windows with `copy_from_user` or `copy_to_user`  | latest |

## run examples
just replace pwn.c with the example you want to run (e.g. ./linux6.6.22/dirty\_cred.c)

then run `./scripts/start-qemu.sh -b` to build and execute `/pwn` inside the vm

## helper scripts:

+ scripts/start-qemu.sh [OPTIONS]\
  -b build and compress rootfs if changed\
  -d build with -DDEBUG\
  -g run with GDB (kaslr still enabled)\
  -k disable kaslr\
  -c force compress rootfs

+ scripts/decompress.sh\
  run this to extract the rootfs.cpio.gz into ./rootfs

+ scripts/compress.sh\
  recompress ./rootfs into rootfs.cpio.gz (i.e. after changes were made)

+ scripts/build.sh\
  build the exploit (pwn.c), and add it to the root of the filesystem /pwn


only using scripts/start-qemu.sh should be sufficient in most cases

## buildroot
compile and modify kernel using buildroot

1. download [buildroot](https://buildroot.org/download.html) and extract
2. apply buildroot keap.patch using patch:
```bash
patch -p1 -i buildroot/keap.patch -d ./PATH/TO/BUIDLROOT
```
3. make changes using `make menuconfig` (e.g. changing kernel version)
4. compile keap and kernel using `make` (might take a while)
5. the final files (rootfs.cpio.gz and bzImage) are located inside the buildroot dir inside `./output/images`

## helpful links
+ bootlin: https://elixir.bootlin.com/linux/v6.6.22/source
