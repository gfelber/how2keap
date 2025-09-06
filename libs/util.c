#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/syscall.h>
#include <unistd.h>

int stage = 0;
volatile int is_busy = 1;

#ifdef x86

uint64_t __attribute__((always_inline)) inline rdtsc() {
  uint64_t a, d;
  asm volatile("mfence");
#if TIMER == TIMER_RDTSCP
  asm volatile("rdtscp" : "=a"(a), "=d"(d)::"rcx");
#elif TIMER == TIMER_RDTSC
  asm volatile("rdtsc" : "=a"(a), "=d"(d));
#elif TIMER == TIMER_RDPRU
  asm volatile(RDPRU : "=a"(a), "=d"(d) : "c"(RDPRU_ECX_APERF));
#endif
  a = (d << 32) | a;
  asm volatile("mfence");
  return a;
}

void __attribute__((always_inline)) inline prefetch(void *p) {
  asm volatile("prefetchnta [%0]\n"
               "prefetcht0 [%0]\n"
               "prefetcht2 [%0]\n"
               "prefetcht2 [%0]\n"
               :
               : "r"(p));
}

size_t flushandreload(void *addr) {
  size_t time = rdtsc();
  prefetch(addr);
  size_t delta = rdtsc() - time;
  return delta;
}

void burn_cycles(unsigned long long cycles) {
  unsigned long long start, ts;
  start = rdtsc();
  do {
    ts = rdtsc();
  } while ((ts - start) < cycles);
}

void *busy_func(void *cpu_ptr) {
  pin_cpu(0, (intptr_t)cpu_ptr);
  while (is_busy)
    burn_cycles(0x1000);
  return NULL;
}

#else

void *busy_func(void *cpu_ptr) {
  pin_cpu(0, (intptr_t)cpu_ptr);
  while (is_busy)
    ;

  return NULL;
}

#endif

void set_priority(pid_t pid, int prio) {
  errno = 0;
  if (setpriority(PRIO_PROCESS, pid, prio) == -1 && errno) {
    fprintf(stderr, "setpriority: %s\n", strerror(errno));
    exit(1);
  }
}

int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param) {
  return syscall(SYS_sched_setscheduler, pid, policy, param);
}

void set_scheduler(pid_t pid, int policy, int prio) {
  struct sched_param param = {
      .sched_priority = policy == SCHED_IDLE ? 0 : prio,
  };
  SYSCHK(sched_setscheduler(pid, policy, &param));
}

int event_create(void) { return SYSCHK(eventfd(0, EFD_SEMAPHORE)); }

void event_signal(int evfd) { SYSCHK(eventfd_write(evfd, 1)); }

void event_wait(int evfd) {
  eventfd_t event;
  SYSCHK(eventfd_read(evfd, &event));
}

void pin_cpu(pid_t pid, int cpu) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);
  SYSCHK(sched_setaffinity(pid, sizeof(cpuset), &cpuset));
}

int rlimit_increase(int rlimit) {
  struct rlimit r;
  SYSCHK(getrlimit(rlimit, &r));

  if (r.rlim_max <= r.rlim_cur) {
    linfo("rlimit %d remains at %.ld", rlimit, r.rlim_cur);
    return 0;
  }

  r.rlim_cur = r.rlim_max;
  int res;
  res = SYSCHK(setrlimit(rlimit, &r));

  ldebug("rlimit %d increased to %lld", rlimit, r.rlim_max);

  return res;
}

int ipow(int base, unsigned int power) {
  int out = 1;
  for (int i = 0; i < power; ++i) {
    out *= base;
  }
  return out;
}

char *cyclic_gen(char *buf, int length) {
  char charset[] = "abcdefghijklmnopqrstuvwxyz";
  int size = length;

  for (int i = 0; length > 0; ++i) {
    buf[size - length] = charset[i % strlen(charset)];
    if (--length == 0)
      break;
    buf[size - length] = charset[(i / strlen(charset)) % strlen(charset)];
    if (--length == 0)
      break;
    buf[size - length] =
        charset[(i / ipow(strlen(charset), 2)) % strlen(charset)];
    if (--length == 0)
      break;
    buf[size - length] =
        charset[(i / ipow(strlen(charset), 3)) % strlen(charset)];
    --length;
  }
  buf[size] = 0;

  return buf;
}

char *cyclic(int length) {
  char *buf = calloc(length + 1, 1);
  return cyclic_gen(buf, length);
}

char *to_hex(char *dst, char *src, size_t size) {
  for (size_t i = 0; i < size; ++i)
    sprintf(dst + i * 2, "%02x", src[i]);
  return dst;
}

void print_hex(void *_data, size_t size) {
  unsigned char *data = (unsigned char *)_data;
  char ascii[16 * 12 + 1];
  size_t i, j, k;
  int skip = 0;
  char *hex;
  ascii[16] = '\0';
  linfo("hexdump: ");

  for (i = 0; i < size; ++i) {
    if (i % 0x10 == 0) {
      bzero(ascii, sizeof(ascii));
      k = 0;
      if (i != 0 && memcmp(&data[i - 0x10], &data[i], 0x10) == 0) {
        i += 0xf;
        if (!skip)
          printf("*\n");
        skip = 1;
        continue;
      } else {
        skip = 0;
        printf("%08lx  ", i);
      }
    }

    if ((data[i] >= ' ') && (data[i] <= '~')) {
      k += sprintf(&ascii[k], GREEN("%c"), ((unsigned char *)data)[i]);
      printf(GREEN("%02x"), data[i]);
    } else {
      if (data[i] == 0xff) {
        printf(BLUE("%02x"), data[i]);
        k += sprintf(&ascii[k], BLUE("."));
      } else if (data[i] == 0) {
        printf("%02x", data[i]);
        k += sprintf(&ascii[k], ".");
      } else {
        printf(RED("%02x"), data[i]);
        k += sprintf(&ascii[k], RED("."));
      }
    }

    if (i % 2 == 1)
      putchar(' ');

    if ((i + 1) % 8 == 0 || i + 1 == size) {
      printf(" ");
      if ((i + 1) % 16 == 0) {
        printf(" %s \n", ascii);
      } else if (i + 1 == size) {
        if ((i + 1) % 16 <= 8) {
          printf(" ");
        }
        for (j = (i + 1) % 16; j < 16; ++j) {
          printf("  ");
          if (j % 2 == 1)
            putchar(' ');
        }
        printf(" %s \n", ascii);
      }
    }
  }
  printf("%08lx\n", i);
}
