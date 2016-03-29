#include "mailslot.h"

#include <linux/fs.h>
#include <linux/errno.h>

static const char* M_NAME = "mailslot";
static unsigned int major = 0;

static slot_t slots[MAX_SLOTS];
static size_t SLOT_SIZE = 4194304;

struct file_operations fops = {
    .open = mailslot_open,
    .read = mailslot_read,
    .write = mailslot_write
};

int mailslot_open(struct inode * inode, struct file * filp){
    unsigned int minor;
    int ret;
    slot_t* currSlot;
    mode_t accmode;

    minor = iminor(filp->f_inode);
    currSlot = slots+minor;

    accmode = filp->f_flags & O_ACCMODE;
    if( (accmode & ( O_WRONLY | O_RDWR)) || !(accmode & O_NONBLOCK)){
        if( !slot_initialized(currSlot) ){
            log_info("Initializing slot: %d", minor);
            ret = slot_init(currSlot, SLOT_SIZE);
            if(ret){
                log_err("Cannot initialize slot");
                return ret;
            }
        }
    }
    return 0;
}

ssize_t mailslot_read(struct file * f, char __user * user, size_t size, loff_t * fpos){
    int ret;
    unsigned int copied;
    unsigned int minor;
    slot_t* currSlot;

    minor = iminor(f->f_inode);
    currSlot = slots+minor;

    log_info("Read operation on slot %d", minor);

    if( !slot_initialized(currSlot) ){
        log_info("Slot %d is not initialized yet", minor);
        return 0;
    }

    ret = slot_to_user(currSlot, user, size, &copied, f->f_flags & O_NONBLOCK);
    if(ret){
        log_err("Error while copying to user buffer");
        return ret;
    }
    return copied;
}

ssize_t mailslot_write(struct file * f, const char __user * user, size_t size, loff_t * loff_t){
    int ret;
    unsigned int copied;
    unsigned int minor;
    slot_t* currSlot;

    minor = iminor(f->f_inode);
    currSlot = slots+minor;
    log_info("Write operation on slot %d, of bytes %u", minor, (unsigned int)size);

    ret = slot_from_user(currSlot, user, size, &copied);
    if(ret){
        log_err("Error while copying from user buffer");
        return ret;
    }
    return copied;
}

int init_module(void) {
    int res;

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

void cleanup_module(void) {
    int i = 0;
    log_debug("Removing module");
    for(i = 0; i < MAX_SLOTS; i++){
        if( slot_initialized(slots+i) )
            log_debug("Freeing slot number %u", i);
            slot_free(slots+i);
    }
    log_debug("Unregistering char device");
    unregister_chrdev(major, M_NAME);
}
