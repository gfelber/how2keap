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

flag is in `/flag` or `/dev/sda`

exploit is located inside the vm in /pwn (recommend running with `while ! /pwn; do true; done`)

## Disclaimer

This source code is provided for **educational and ethical purposes** only. The author(s) strictly prohibit any use of this code for unlawful, malicious, or unauthorized activities. By using this code, you agree to comply with all applicable laws and take full responsibility for any misuse.

The author(s) disclaim all liability for damages or legal consequences resulting from improper or illegal use of this code. Use responsibly and only in accordance with ethical guidelines and legal requirements.

## Techniques

### Privilige Escalation

| File                          | Technique                                                    | Linux-Version | Jail Escape | Applicable CTF Challenges                             |
| - | - | - | - | - |
| [mad\_cow.c](/linux6.12.27/mad\_cow.c) | corrupt file struct and abuse COW to inject code into another process | latest        | ~[^1] | [Baby VMA](https://github.com/ECSC2024/ECSC2024-CTF-Jeopardy/tree/main/pwn05) |
| [dirty\_cred.c](/linux6.12.27/dirty_cred.c) | [DirtyCred](https://github.com/Markakd/DirtyCred) abuses the heap memory reuse mechanism to get privileged, using MadCOW to achive privilege escalation  | latest          | ~[^2] | [Wall Rose](https://ctf2023.hitcon.org/dashboard/#15) |
| [dirty\_pagetable.c](/linux6.12.27/dirty_pagetable.c) | [Dirty Pagetable](https://yanglingxi1993.github.io/dirty_pagetable/dirty_pagetable.html) abuse pagetables to get unprotected AAR/AAW in kernel space (kernel RCE) | latest        | X| [keasy](https://ptr-yudai.hatenablog.com/entry/2023/12/08/093606#Dirty-Pagetable) |

[^1]: Jail escape possible if a binary or library is accessible as readonly in jail and used by outside process
[^2]: Jail escape only possible through MadCOW style escape

### Gadgets
| File                          | Technique                                                    | Linux-Version | Applicable CTF Challenges                             |
| - | - | - | - |
| [cross\_cache.c](/linux6.12.27/cross_cache.c) | showcasing a cross cacheq attack that allows using dangeling ptrs to target heap of other slabs | latest  | [Wall Rose](https://ctf2023.hitcon.org/dashboard/#15)
| [per\_cpu\_slabs.c](/linux6.12.27/per_cpu_slabs.c) | showcasing how slabs are managed and reallocated on a per cpu basis| latest  |
| [mmaped\_files.c](/linux6.12.27/mmaped_files.c) |   using mmaped files to create race windows with `copy_from_user` or `copy_to_user`  | latest |
| [entrybleed.c](/linux6.12.27/entrybleed.c) | EntryBleed exploit prefetch timing to leak KASLR | latest  | |
| [slubstick.c](/linux6.12.27/slubstick.c) | [SLUBStick](https://github.com/IAIK/SLUBStick) more reliable way to trigger cross-cache  | latest        |  |


## run examples
just replace pwn.c with the example you want to run (e.g. ./linux6.6.22/dirty\_cred.c)

then run `./scripts/start-qemu.sh -b` to build and execute `/pwn` inside the vm

## helper scripts:

+ scripts/start-qemu.sh [OPTIONS]\
-b build and compress rootfs if changed \
-d build with -DDEBUG \
-p run /pwn on startup \
-g run with GDB (kaslr still enabled) \
-k disable kaslr \
-c force compress rootfs \
-l set loglevel to 9 \
-r run as root \
-n run with nsjail \
-v run with kvm (host cpu)

+ scripts/gdbinit\
  This file can be used to specify gdb commands to be executed on startup.

## buildroot
compile and modify kernel using buildroot

1. download [buildroot](https://buildroot.org/download.html) and extract
2. apply buildroot keap.patch and config.patch using patch:
```bash
patch -p1 -i buildroot/keap.patch -d ./PATH/TO/BUIDLROOT
make -C ./PATH/TO/BUIDLROOT qemu_x86_64_defconfig
patch buildroot/.config ./PATH/TO/BUIDLROOT/config.patch
```
3. make changes using `make menuconfig` (e.g. changing kernel version)
4. compile keap and kernel using `make` (might take a while)
5. you will be prompted about additional packages, default is fine
5. the final files (rootfs.cpio.gz and bzImage) are located inside the buildroot dir inside `./output/images`

## helpful links
+ bootlin: https://elixir.bootlin.com/linux/v6.10.10/source
