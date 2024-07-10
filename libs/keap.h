#include <stddef.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "util.h"

#ifndef KEAP_H
#define KEAP_H

#define KEAP_IOCTL_MALLOC _IOW('K', 0, struct keap_malloc_param *)
#define KEAP_IOCTL_READ   _IOW('K', 1, struct keap_transfer *)
#define KEAP_IOCTL_WRITE  _IOW('K', 2, struct keap_transfer *)
#define KEAP_IOCTL_FREE   _IOW('K', 3, void *)

struct keap_malloc_param {
  unsigned int flags;
  size_t size;
  void* heap_ptr;
};

struct keap_transfer {
  void* heap_ptr;
  void* buf;
  size_t size;
};


extern int keap_fd;
void init();
void* keap_malloc(size_t size, int flags);
long keap_read(void *ptr, void *buf, size_t size);
long keap_write(void* ptr, void *buf, size_t size);
void keap_free(void *ptr);

#endif
