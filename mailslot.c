#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include "mailslot.h"

static const char* M_NAME = "mailslot";
static unsigned int major = 0;
static int res;
static void * module_buff;
static const size_t TOT_BUFF_SIZE = 1024;
static size_t  last_written_size = 0;

struct file_operations fops = {
    .read = mailslot_read,
    .write = mailslot_write
};

int init_module(void) {
    printk(KERN_DEBUG "%s: registering module\n", M_NAME);

    res = register_chrdev(major, M_NAME, &fops);
    if (res < 0) {
        printk(KERN_WARNING "%s: can’t register char devices driver\n", M_NAME);
        return res;
    }
    if (major == 0){
        major = res;
        printk(KERN_INFO "%s: assigned major number: %d\n", M_NAME, major);
    }

    module_buff = kmalloc(TOT_BUFF_SIZE, GFP_KERNEL);
    if(!module_buff){
        printk(KERN_ERR "%s: cannot allocate module buffer", M_NAME);
        return 1;
    }

    return 0;
}


// ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
ssize_t mailslot_read(struct file * f, char __user * user, size_t size, loff_t * fpos){
    unsigned minor;
    unsigned long not_written;
    size_t resp_size;

    printk(KERN_INFO "%s: read operation\n", M_NAME);
    minor = iminor(f->f_inode);
    printk(KERN_INFO "%s: minor %d\n", M_NAME, minor);
    printk(KERN_INFO "%s: loff_t %lld\n", M_NAME, *fpos);
    if(*fpos >= last_written_size)
        return 0;
    if(size < last_written_size)
        resp_size = size;
    else
        resp_size = last_written_size;
    not_written = copy_to_user(user, module_buff, resp_size);
    if(not_written){
        printk(KERN_ERR "%s: cannot copy data from to user space memory %ld\n", M_NAME, not_written);
        return -EIO;
    }
    *fpos += resp_size;
    return resp_size;
}

// ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
ssize_t mailslot_write(struct file * f, const char __user * user, size_t size, loff_t * loff_t){
    unsigned minor;
    unsigned long not_written;
    printk(KERN_INFO "%s: write operation\n", M_NAME);
    minor = iminor(f->f_inode);
    printk(KERN_INFO "%s: minor %d\n", M_NAME, minor);
    if(size > TOT_BUFF_SIZE){
        printk(KERN_ERR "%s: cannot handle such a size %ld\n", M_NAME, size);
        return -1;
    }
    not_written = copy_from_user(module_buff, user, size);
    if(not_written){
        printk(KERN_ERR "%s: cannot copy data from user space memory\n", M_NAME);
        return -EIO;
    }
    last_written_size = size;
    return size;
}


void cleanup_module(void) {
    printk(KERN_DEBUG "%s: Removing module\n", M_NAME);
    unregister_chrdev(major, M_NAME);
}
