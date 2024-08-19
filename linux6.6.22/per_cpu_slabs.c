#include "libs/pwn.h"
#include <unistd.h>

/*******************************
 * EXPLOIT                     *
 *******************************/

#define CHUNK_SIZE 0x100

int main(int argc, char *argv[]) {
  void *freed_ptr, *diff_ptr, *same_ptr;

  setvbuf(stdout, NULL, _IONBF, 0);

  lstage("INIT");

  pin_cpu(0, 0);
  init();

  lstage("START");

  linfo("allocating and freeing chunk");
  pin_cpu(0, 1);

  freed_ptr = keap_malloc(CHUNK_SIZE, GFP_KERNEL_ACCOUNT);
  keap_free(freed_ptr);

  linfo("free done done");

  linfo("trying to realloc from different cpu (Spoiler: will fail)");

  pin_cpu(0, 2);
  diff_ptr = keap_malloc(CHUNK_SIZE, GFP_KERNEL_ACCOUNT);

  CHK(diff_ptr != freed_ptr);

  linfo("trying to realloc from same cpu");

  pin_cpu(0, 1);

  same_ptr = keap_malloc(CHUNK_SIZE, GFP_KERNEL_ACCOUNT);

  CHK(same_ptr == freed_ptr);

  linfo("successfully realloced from same cpu");

  keap_free(diff_ptr);
  keap_free(same_ptr);

  return 0;
}
