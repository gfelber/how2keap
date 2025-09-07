#include "libs/pwn.h"

/*
 * inspired by https://github.com/IAIK/SLUBStick
 */

static size_t *times;

void timed_alloc_objs(size_t size, size_t *times) {
  int ret;
  size_t t0;
  size_t t1;

  linfo("allocate %ld objs", cur->allocs);
  for (size_t i = 0; i < cur->allocs; ++i) {
    sched_yield();
    t0 = rdtsc();
    keap_malloc(cur->size, GFP_KERNEL);
    t1 = rdtsc();
    times[i] = t1 - t0;
  }
}

#define MAX_TIME_VAL 128
void print_times(size_t *times, size_t size) {
  size_t max = 0;
  size_t min = -1;
  size_t adjusted_time;

  for (size_t i = 0; i < size; ++i) {
    if (times[i] > max)
      max = times[i];
    if (times[i] < min)
      min = times[i];
  }

  linfo("min: %ld, max: %ld", min, max);
  for (size_t i = 0; i < size; ++i) {
    adjusted_time = (times[i] - min) * MAX_TIME_VAL / max;
    printf("% 5ld:% 7ld:", i, times[i]);
    for (size_t j = 0; j < adjusted_time; ++j)
      putchar('#');
    putchar('\n');
  }
}

int main(int argc, char *argv[]) {
  size_t slab_size = argc < 2 ? 128 : strtol(argv[1], 0, 10);

  lstage("init");
  init();
  pin_cpu(0, 0);
  set_current_slab_info(slab_size);
  times = calloc(cur->allocs, sizeof(size_t));

  lstage("start");

  lstage("%ld bytes", slab_size);
  timed_alloc_objs(slab_size, times);

  lstage("print times");
  print_times(times, cur->allocs);
  free(times);
}
