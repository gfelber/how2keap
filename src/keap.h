#ifndef _KEAP_H
#define _KEAP_H

#define DEVICE_NAME "keap"
#define CLASS_NAME  DEVICE_NAME

struct keap_malloc_param {
  unsigned int flags;
  size_t size;
  void* heap_ptr;
};

struct keap_transfer {
  void* heap_ptr;
  void __user *  buf;
  size_t size;
};

struct chrdev_info {
	unsigned int major;
	struct cdev cdev;
	struct class *class;
};

#define KEAP_IOCTL_MALLOC _IOW('K', 0, struct keap_malloc_param *)
#define KEAP_IOCTL_READ   _IOW('K', 1, struct keap_transfer *)
#define KEAP_IOCTL_WRITE  _IOW('K', 2, struct keap_transfer *)
#define KEAP_IOCTL_FREE   _IOW('K', 3, void *)

#endif
