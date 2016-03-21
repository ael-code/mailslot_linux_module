#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/kfifo.h>
#include <linux/kdev_t.h>
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

#define LOG(lvl, msg, ...) printk(lvl "%s: "msg"\n", KBUILD_MODNAME, ##__VA_ARGS__)
#define log_debug(msg, ...) LOG(KERN_DEBUG, msg, ##__VA_ARGS__)
#define log_err(msg, ...) LOG(KERN_ERR, msg, ##__VA_ARGS__)
#define log_info(msg, ...) LOG(KERN_INFO, msg, ##__VA_ARGS__)

int init_module(void) {
    log_debug("registering module");
    log_debug("Maximum mailslots allowed: %d", 1 << MINORBITS);
    log_debug("Size of kfifo: %ld", sizeof(struct __kfifo));
    res = register_chrdev(major, M_NAME, &fops);
    if (res < 0) {
        log_err("can’t register char devices driver");
        return res;
    }
    if (major == 0){
        major = res;
        log_info("assigned major number: %d", major);
    }

    module_buff = kmalloc(TOT_BUFF_SIZE, GFP_KERNEL);
    if(!module_buff){
        log_err("cannot allocate module buffer");
        return 1;
    }

    return 0;
}


// ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
ssize_t mailslot_read(struct file * f, char __user * user, size_t size, loff_t * fpos){
    unsigned minor;
    unsigned long not_written;
    size_t resp_size;

    log_info("read operation");
    minor = iminor(f->f_inode);
    log_info("minor %d", minor);
    log_info("loff_t %lld", *fpos);
    if(*fpos >= last_written_size)
        return 0;
    if(size < last_written_size)
        resp_size = size;
    else
        resp_size = last_written_size;
    not_written = copy_to_user(user, module_buff, resp_size);
    if(not_written){
        log_err("cannot copy data from to user space memory %ld", not_written);
        return -EIO;
    }
    *fpos += resp_size;
    return resp_size;
}

// ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
ssize_t mailslot_write(struct file * f, const char __user * user, size_t size, loff_t * loff_t){
    unsigned minor;
    unsigned long not_written;
    log_info("write operation");
    minor = iminor(f->f_inode);
    log_info("minor %d", minor);
    if(size > TOT_BUFF_SIZE){
        log_err("cannot handle such a size %ld", size);
        return -1;
    }
    not_written = copy_from_user(module_buff, user, size);
    if(not_written){
        log_err("cannot copy data from user space memory");
        return -EIO;
    }
    last_written_size = size;
    return size;
}


void cleanup_module(void) {
    log_debug("Removing module");
    unregister_chrdev(major, M_NAME);
}
