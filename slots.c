#include "slots.h"

int slot_init(slot_t* slot, size_t slot_size){
    int ret;

    kfifo_rec* fifo = &slot->fifo;
    if( !kfifo_initialized(fifo) ){
        log_err("Slot already initialized");
        return 1;
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

/**
int slot_from_user(struct slot, const void __user * buf, size_t len, size_t * copied){

    ret = kfifo_from_user(currSlot, user, size, &copied);
    if( (!ret) && (!copied) ){
        log_err("Not enough space available on slot");
        return -ENOSPC;
    }
    if(ret){
        log_err("Cannot copy from user space");
        return ret;
    }

}
**/
//int slot_to_user(struct slot, void __user * buf, size_t len, size_t * copied);
