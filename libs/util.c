#include "util.h"


void burn_cycles(unsigned long long cycles)
{
	unsigned long long start, ts;
	start = RDTSC();
	do {
		ts = RDTSC();
	} while ((ts - start) < cycles);
}

void pin_cpu(pid_t pid, int cpu)
{
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);
	SYSCHK(sched_setaffinity(pid, sizeof(cpuset), &cpuset));
}

int rlimit_increase(int rlimit)
{
  struct rlimit r;
  if (getrlimit(rlimit, &r)) {
    puts("rlimit_increase:getrlimit");
    exit(EXIT_FAILURE);
  }

  if (r.rlim_max <= r.rlim_cur)
  {
      printf("[+] rlimit %d remains at %.lld", rlimit, r.rlim_cur);
      return 0;
  }
  r.rlim_cur = r.rlim_max;
  int res;
  if (res = setrlimit(rlimit, &r)) {
    puts("rlimit_increase:setrlimit");
    exit(EXIT_FAILURE);
  }
  else
    printf("[+] rlimit %d increased to %lld\n", rlimit, r.rlim_max);
  return res;
}

int ipow(int base, unsigned int power){
    int out = 1;
    for (int i = 0; i < power; ++i) {
        out *= base;
    }
    return out;
}

char* cyclic(int length) {
    char charset[] = "abcdefghijklmnopqrstuvwxyz";
    int size = length;
    char* pattern = malloc(size+1);

    for (int i = 0; length > 0; ++i) {
        pattern[size - length] = charset[i % strlen(charset)];
        if (--length == 0) break;
        pattern[size - length] = charset[(i / strlen(charset)) % strlen(charset)];
        if (--length == 0) break;
        pattern[size - length] = charset[(i / ipow(strlen(charset), 2)) % strlen(charset)];
        if (--length == 0) break;
        pattern[size - length] = charset[(i / ipow(strlen(charset), 3)) % strlen(charset)];
        --length;
    }
    pattern[size] = 0;

    return pattern;
}

void print_hex(char* buf, int len){
    for (int i = 0; i < len; ++i) {
        if (i % 8 == 0) puts("");
        printf("%02hhx", buf[i]);
    }
    puts("");
}


