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

#define MAX_SLOTS (1 << MINORBITS)
typedef struct kfifo_rec_ptr_2 slot;
static slot slots[MAX_SLOTS];
static size_t SLOT_SIZE = 1024;

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
    log_debug("Maximum mailslots allowed: %d", MAX_SLOTS);
    res = register_chrdev(major, M_NAME, &fops);
    if (res < 0) {
        log_err("can’t register char devices driver");
        return res;
    }
    if (major == 0){
        major = res;
        log_info("assigned major number: %d", major);
    }

    return 0;
}


ssize_t mailslot_read(struct file * f, char __user * user, size_t size, loff_t * fpos){
    int ret;
    unsigned int copied;
    unsigned minor;
    slot* currSlot;

    minor = iminor(f->f_inode);
    currSlot = slots+minor;

    log_info("Read operation on slot %d", minor);

    if( !kfifo_initialized(slots+minor) ){
        log_info("Slot %d is not initialized yet", minor);
        return 0;
    }

    log_info("next item length: %d", kfifo_peek_len(currSlot));
    if( kfifo_peek_len(currSlot) > size){
        log_err("Cannot read mail, destination buffer too short");
        return -EIO;
    }

    ret = kfifo_to_user(currSlot, user, size, &copied);
    if(ret){
        log_err("Error while reading");
        return ret;
    }
    log_info("Copied %d byte to user buffer", copied);
    return copied;
}

ssize_t mailslot_write(struct file * f, const char __user * user, size_t size, loff_t * loff_t){
    int ret;
    unsigned int copied;
    unsigned minor;
    slot* currSlot;

    minor = iminor(f->f_inode);
    currSlot = slots+minor;
    log_info("Write operation on slot %d, of bytes %u", minor, (unsigned int)size);

    if( !kfifo_initialized(currSlot) ){
        log_info("Initializing slot: %d", minor);
        ret = kfifo_alloc(currSlot, SLOT_SIZE, GFP_KERNEL);
        if(ret){
            log_err("Cannot allocate new slot");
            return ret;
        }
    }

    ret = kfifo_from_user(currSlot, user, size, &copied);
    if( (!ret) && (!copied) ){
        log_err("Not enough space available on slot");
        return -ENOSPC;
    }
    if(ret){
        log_err("Cannot copy from user space");
        return ret;
    }
    log_info("Copied from user (ret %d), (copied %d)", ret, copied);

    return copied;
}


void cleanup_module(void) {
    int i = 0;
    log_debug("Removing module");
    for(i = 0; i < MAX_SLOTS; i++){
        if( kfifo_initialized(slots+i) )
            log_debug("Freeing slot number %u", i);
            kfifo_free(slots+i);
    }
    unregister_chrdev(major, M_NAME);
}
