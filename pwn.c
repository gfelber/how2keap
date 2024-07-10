#include "libs/pwn.h"


/*******************************
 * EXPLOIT                     *
 *******************************/


int main(int argc, char* argv[]) {
  char* msg = "Hello World";
  char buf[strlen(msg)+1];
  bzero(buf, sizeof(buf));
  
  init();

  void* ptr = keap_malloc(strlen(msg), GFP_KERNEL_ACCOUNT);
  printf("ptr: %p\n", ptr);
  keap_write(ptr, msg, strlen(msg));
  keap_read(ptr, buf, strlen(msg));
  keap_free(ptr);

  puts(buf);

  return 0;  
}
