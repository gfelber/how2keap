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
	fprintf(stderr, "%s\n", "CHK(" #x ")"); \
	exit(1); } } while (0)

/* Assert that a syscall x has succeeded. */
#define SYSCHK(x) ({ \
	typeof(x) __res = (x); \
	if (__res == (typeof(x))-1) { \
		fprintf(stderr, "%s: %s\n", "SYSCHK(" #x ")", strerror(errno)); \
		exit(1); \
	} \
	__res; \
})


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
void print_hex(char* buf, int len);

#endif
