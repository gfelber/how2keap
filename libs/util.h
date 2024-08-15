#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/resource.h>

#ifndef UTIL_H
#define UTIL_H

/* Assert that x is true. */
#define CHK(x) do { if (!(x)) { \
	lerror("%s\n", "CHK(" #x ")"); } } while (0)

/* Assert that a syscall x has succeeded. */
#define SYSCHK(x) ({ \
	typeof(x) __res = (x); \
	if (__res == (typeof(x))-1) { \
		lerror("%s: %s\n", "SYSCHK(" #x ")", strerror(errno)); \
	} \
	__res; \
})


/* MACRO to enforce inline */
#define RDTSC() ({ \
    uint32_t lo, hi; \
    __asm__ __volatile__ ( \
      "xorl %%eax, %%eax\n" \
      "lfence\n" \
      "rdtsc\n" \
      : "=a" (lo), "=d" (hi) \
      : \
      : "%ebx", "%ecx"); \
    (uint64_t)hi << 32 | lo; \
})


void burn_cycles(unsigned long long cycles);
void pin_cpu(pid_t pid, int cpu);
int rlimit_increase(int rlimit);

int ipow(int base, unsigned int power);
char* cyclic(int length);

/* logging */
void print_hex(char* buf, int len);

#define LINFO "[\033[34;1m*\033[0m] "
#define LDEBUG "[\033[32;1mD\033[0m] "
#define LWARN "[\033[33;1m!\033[0m] "
#define LERROR "[\033[31;1m-\033[0m] "
#define LSTAGE "[\033[35;1mSTAGE: %d\033[0m] "

#define linfo(format, ...) printf (LINFO format "\n", ##__VA_ARGS__)

#define lwarn(format, ...) fprintf (stderr, LWARN format "\n", ##__VA_ARGS__)

#define lerror(format, ...) fprintf (stderr, LERROR format "\n", ##__VA_ARGS__)

extern int stage;
#define lstage(format, ...) printf (LSTAGE format "\n", ++stage, ##__VA_ARGS__)

#ifdef DEBUG
#define ldebug(format, ...) printf (LDEBUG format "\n", ##__VA_ARGS__)
#else
#define ldebug(...)
#endif

#endif
