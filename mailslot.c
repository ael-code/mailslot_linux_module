#include "mailslot.h"

#include <linux/fs.h>
#include <linux/errno.h>

static const char* M_NAME = "mailslot";
static unsigned int major = 0;

static slot_t slots[MAX_SLOTS];

struct file_operations fops = {
    .open = mailslot_open,
    .read = mailslot_read,
    .write = mailslot_write,
    .release = mailslot_release,
    .unlocked_ioctl = mailslot_unlocked_ioctl
};

int mailslot_open(struct inode * inode, struct file * filp){
    unsigned int minor;
    int ret;
    slot_t* currSlot;
    mode_t accmode;

    try_module_get(THIS_MODULE);
    minor = iminor(filp->f_inode);
    currSlot = slots+minor;

    accmode = filp->f_flags & O_ACCMODE;
    if( (accmode & ( O_WRONLY | O_RDWR)) || !(accmode & O_NONBLOCK)){
        if( !slot_initialized(currSlot) ){
            log_info("Initializing slot: %d", minor);
            ret = slot_init(currSlot, DEFAULT_SLOT_SIZE);
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

int mailslot_release(struct inode * inode, struct file * f){
    module_put(THIS_MODULE);
    return 0;
}


long mailslot_unlocked_ioctl(struct file * f, unsigned int cmd, unsigned long arg){
    unsigned int want_non_blocking;
    unsigned int size;
    long ret = 0;

    switch(cmd){
        case MAILSLOT_IOC_SET_NONBLOCKING:
            log_debug("Setting blocking mode: %u", *((unsigned int * __user )arg));
            if ((ret = get_user(want_non_blocking, (unsigned int __user *)arg)) != 0)
                break;
            if(want_non_blocking)
               f->f_flags |= O_NONBLOCK;
            else
               f->f_flags &= ~O_NONBLOCK;
            break;

        case MAILSLOT_IOC_IS_NONBLOCKING:
            log_debug("Requesting blocking mode: %u", f->f_flags & O_NONBLOCK);
            ret = put_user(f->f_flags & O_NONBLOCK, (unsigned int __user *)arg);
            break;

        case MAILSLOT_IOC_RESIZE:
            if ((ret = get_user(size, (unsigned int __user *)arg)) != 0)
                break;
            log_debug("Request to resize: %u", size);
            ret = slot_resize(slots+iminor(f->f_inode), size);
            if(ret) log_err("Failed to resize");
            break;

        default: /* redundant, as cmd was checked against MAXNR */
            return -ENOTTY;
    }

    return ret;
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
