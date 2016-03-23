#include "slots.h"

int slot_init(slot_t* slot, size_t slot_size){
    int ret;

    kfifo_rec* fifo = &slot->fifo;
    if( kfifo_initialized(fifo) ){
        log_err("Slot already initialized");
        return -1;
    }

    log_debug("Initializing kfifo buffer");
    ret = kfifo_alloc(fifo, slot_size, GFP_KERNEL);
    if(ret){
        log_err("Cannot allocate new slot");
        return ret;
    }

    log_debug("Initializing spinlocks");
    mutex_init(&slot->r_lock);
    mutex_init(&slot->w_lock);

    return 0;
}

void slot_free(slot_t* slot){
    //It is necessary to wait for all reader/writer to finish?
    mutex_destroy(&slot->r_lock);
    mutex_destroy(&slot->w_lock);
    kfifo_free(&slot->fifo);
}

int slot_initialized(slot_t* slot){
    return kfifo_initialized(&slot->fifo);
}

int slot_from_user(slot_t* slot, const void __user * buf, size_t len, unsigned int* copied){
    int ret;

    ret = kfifo_from_user(&slot->fifo, buf, len, copied);
    if( (!ret) && (!copied) ){
        log_err("Not enough space available on slot");
        return -ENOSPC;
    }
    if(ret){
        log_err("Cannot copy from user space");
        return ret;
    }
    log_info("Copied from user (ret %d), (copied %d)", ret, *copied);
    return 0;
}

int slot_to_user(slot_t* slot, void __user * buf, size_t len, unsigned int* copied){
    int ret;

    ret = kfifo_to_user(&slot->fifo, buf, len, copied);
    if(ret){
        log_err("Error while copying to user buffer");
        return ret;
    }
    log_info("Copied %d byte to user buffer", *copied);
    return 0;
}