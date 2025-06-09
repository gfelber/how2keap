#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>

#ifndef UTIL_H
#define UTIL_H

#if defined(__x86_64__) || defined(__i386__)
#define x86
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

/* Assert that x is true. */
#define CHK(x)                                                                 \
  do {                                                                         \
    if (!(x)) {                                                                \
      lerror("%s\n", "CHK(" #x ")");                                           \
    }                                                                          \
  } while (0)

/* Assert that a syscall x has succeeded. */
#define SYSCHK(x)                                                              \
  ({                                                                           \
    typeof(x) __res = (x);                                                     \
    if (__res == (typeof(x))-1) {                                              \
      lerror("%s:\n  %s", "SYSCHK(" #x ")", strerror(errno));                  \
    }                                                                          \
    __res;                                                                     \
  })

#ifdef x86
/* MACRO to enforce inline */
#define rdtsc()                                                                \
  ({                                                                           \
    uint32_t lo, hi;                                                           \
    __asm__ __volatile__("xor eax, eax\n"                                      \
                         "lfence\n"                                            \
                         "rdtsc\n"                                             \
                         : "=a"(lo), "=d"(hi)                                  \
                         :                                                     \
                         : "ebx", "ecx");                                      \
    (uint64_t) hi << 32 | lo;                                                  \
  })

void burn_cycles(unsigned long long cycles);
#endif

void pin_cpu(pid_t pid, int cpu);
int rlimit_increase(int rlimit);

void set_priority(pid_t pid, int prio);
void set_scheduler(pid_t pid, int policy, int prio);
int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param);

extern volatile int is_busy;
void *busy_func(void *cpu_ptr);

int event_create(void);
void event_signal(int evfd);
void event_wait(int evfd);

int ipow(int base, unsigned int power);
char *cyclic_gen(char *buf, int length);
char *cyclic(int length);
char *to_hex(char *dst, char *src, size_t size);

/* logging */
void print_hex(void *data, size_t size);

#define RED(x) "\033[31;1m" x "\033[0m"
#define GREEN(x) "\033[32;1m" x "\033[0m"
#define YELLOW(x) "\033[33;1m" x "\033[0m"
#define BLUE(x) "\033[34;1m" x "\033[0m"
#define MAGENTA(x) "\033[35;1m" x "\033[0m"

#define LINFO "[" BLUE("*") "] "
#define LDEBUG "[" GREEN("D") "] "
#define LWARN "[" YELLOW("!") "] "
#define LERROR "[" RED("-") "] "
#define LSTAGE "[" MAGENTA("STAGE: %d") "] "

#define linfo(format, ...) printf(LINFO format "\n", ##__VA_ARGS__)
#define lhex(x) linfo("0x%016lx <- %s", (uint64_t)x, #x)

#define lwarn(format, ...) fprintf(stderr, LWARN format "\n", ##__VA_ARGS__)

#define lerror(format, ...)                                                    \
  do {                                                                         \
    fprintf(stderr, LERROR format "\n", ##__VA_ARGS__);                        \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

extern int stage;
#define lstage(format, ...) printf(LSTAGE format "\n", ++stage, ##__VA_ARGS__)

#ifdef DEBUG
#define ldebug(format, ...) printf(LDEBUG format "\n", ##__VA_ARGS__)
#else
#define ldebug(...)
#endif

#endif
