#include "libs/pwn.h"

/*******************************
 * EXPLOIT                     *
 *******************************/

int main(int argc, char *argv[]) {
  char *msg = "Hello World";
  char buf[strlen(msg) + 1];
  bzero(buf, sizeof(buf));

  lstage("INIT");

  init();
  rlimit_increase(RLIMIT_NOFILE);
  pin_cpu(0, 0);

  lstage("START");

  void *ptr = keap_malloc(strlen(msg), GFP_KERNEL_ACCOUNT);
  linfo("ptr: %p", ptr);
  keap_write(ptr, msg, strlen(msg));
  keap_read(ptr, buf, strlen(msg));
  keap_free(ptr);

  linfo("MSG: %s", buf);

  return 0;
}
