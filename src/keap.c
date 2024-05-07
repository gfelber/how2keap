#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>

#include "keap.h"

static long keap_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
  struct keap_transfer transfer;
  struct keap_malloc_param param;

  switch (cmd) {
    case KEAP_IOCTL_MALLOC:
      if (copy_from_user(&param, (void __user *)arg, sizeof(param)))
        return -EFAULT;
      printk("keap: allocate 0x%lx with flags 0x%x\n", param.size, param.flags);
      param.heap_ptr = kmalloc(param.size, param.flags);
      return copy_to_user((void __user *)arg, &param, sizeof(param));

    case KEAP_IOCTL_READ:
      if (copy_from_user(&transfer, (void __user *)arg, sizeof(transfer)))
        return -EFAULT;
      printk("keap: copy %p to %p (user) (size: 0x%lx)\n", transfer.heap_ptr, transfer.buf, transfer.size);
      return copy_to_user((void __user *)transfer.buf, transfer.heap_ptr, transfer.size);

    case KEAP_IOCTL_WRITE:
      if (copy_from_user(&transfer, (void __user *)arg, sizeof(transfer)))
        return -EFAULT;
      printk("keap: copy %p (user) to %p (size: 0x%lx)\n", transfer.buf, transfer.heap_ptr, transfer.size);
      return copy_from_user(transfer.heap_ptr, (void __user *)transfer.buf, transfer.size);

    case KEAP_IOCTL_FREE:
      printk("keap: free %p\n", (void*)arg);
      kfree((void*) arg);
      return 0;
  }

  return -EINVAL;

}

static struct file_operations keap_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = keap_ioctl,
};

static struct miscdevice keap_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "keap",
	.fops = &keap_fops,
};

static int keap_init(void)
{
	pr_info("keap: initialize\n");
	return misc_register(&keap_device);
}

static void keap_exit(void)
{
	pr_info("keap: exit\n");
	misc_deregister(&keap_device);
}

module_init(keap_init);
module_exit(keap_exit);

MODULE_AUTHOR("\x06\xfe\x1b\xe2");
MODULE_DESCRIPTION("keap a debloated heap allocation interface");
MODULE_LICENSE("GPL");
