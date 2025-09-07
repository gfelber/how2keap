#include "keap.h"

int keap_fd = -1;

void init() { keap_fd = SYSCHK(open("/proc/keap", O_RDWR)); }

void *keap_malloc(size_t size, int flags) {
  struct keap_malloc_param param = {
      .flags = flags, .size = size, .heap_ptr = NULL};
  SYSCHK(ioctl(keap_fd, KEAP_IOCTL_MALLOC, &param));

  return param.heap_ptr;
}

long keap_read(void *ptr, void *buf, size_t size) {
  struct keap_transfer transfer = {.heap_ptr = ptr, .buf = buf, .size = size};
  return SYSCHK(ioctl(keap_fd, KEAP_IOCTL_READ, &transfer));
}

long keap_write(void *ptr, void *buf, size_t size) {
  struct keap_transfer transfer = {.heap_ptr = ptr, .buf = buf, .size = size};
  return SYSCHK(ioctl(keap_fd, KEAP_IOCTL_WRITE, &transfer));
}

int keap_free(void *ptr) { return ioctl(keap_fd, KEAP_IOCTL_FREE, ptr); }
