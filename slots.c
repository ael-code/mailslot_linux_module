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

    log_debug("Initializing inqueue");
    init_waitqueue_head(&slot->inq);

    return 0;
}

int slot_resize(slot_t* slot, size_t slot_size){
    int ret;
    void* old_buffer;
    void* new_buffer;
    kfifo_rec* fifo = &slot->fifo;

    slot_size = roundup_pow_of_two(slot_size);
    if (slot_size < 2) {
        log_err("Cannot shrink: invalid dimension");
        return -1;
    }

    if( !kfifo_initialized(fifo) ){
        log_err("Cannot resize uninitialized kfifo");
        return -1;
    }

    if(mutex_lock_interruptible(&slot->w_lock) || mutex_lock_interruptible(&slot->r_lock))
        return -ERESTARTSYS;

    if(slot_size < kfifo_len(fifo)){
        mutex_unlock(&slot->r_lock); mutex_unlock(&slot->w_lock);
        log_err("Cannot shrink to a dimension less then the used one");
        return -1;
    }

    new_buffer = kmalloc(slot_size, GFP_KERNEL);
    if(!new_buffer){
        mutex_unlock(&slot->r_lock); mutex_unlock(&slot->w_lock);
        log_err("Cannot allocate new buffer");
        return -ENOMEM;
    }

    log_debug("Shrinking old data -> size: %u, len: %u, in: %u, out: %u", kfifo_size(fifo), kfifo_len(fifo), fifo->kfifo.in, fifo->kfifo.out);

    old_buffer = fifo->kfifo.data;
    ret = __kfifo_out_peek(&(fifo->kfifo), new_buffer, kfifo_size(fifo));
    fifo->kfifo.data = new_buffer;
    fifo->kfifo.in = kfifo_len(fifo);
    fifo->kfifo.out = 0;
    fifo->kfifo.mask = slot_size - 1;

    log_debug("Shrinking new data -> size: %u, len: %u, in: %u, out: %u", kfifo_size(fifo), kfifo_len(fifo), fifo->kfifo.in, fifo->kfifo.out);

    mutex_unlock(&slot->r_lock); mutex_unlock(&slot->w_lock);

    kfree(old_buffer);
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
* Write a message in the given mailslot taken from the user buffer @buf.
*/
int slot_from_user(slot_t* slot, const void __user * buf, size_t len, unsigned int* copied){
    int ret;

    if(len > RECORD_SIZE){
        log_err("Request size is %d but the maximum allowed is %d", (unsigned int)len, RECORD_SIZE);
        return -EIO;
    }

    /** Critical section **/
    if(mutex_lock_interruptible(&slot->w_lock))
        return -ERESTARTSYS;

    ret = kfifo_from_user(&slot->fifo, buf, len, copied);

    mutex_unlock(&slot->w_lock);
    /** End critical section **/

    if( ! (ret | *copied) ){ //if both are 0
        log_err("Not enough space available on slot");
        return -ENOSPC;
    }
    if(ret){
        log_err("Cannot copy from user space");
        return ret;
    }

    wake_up_interruptible(&slot->inq);
    log_info("Copied from user (ret %d), (copied %d)", ret, *copied);
    return 0;
}

/**
* Read a message from the given mailslot and copy it to the user buffer @buf.
*/
int slot_to_user(slot_t* slot, void __user * buf, size_t len, unsigned int* copied, unsigned short non_block){
    int ret;


    /** Critical section **/
    if(mutex_lock_interruptible(&slot->r_lock))
        return -ERESTARTSYS;

    while (kfifo_is_empty(&slot->fifo)) { // nothing to read
        mutex_unlock(&slot->r_lock); //release the lock
        if (non_block)
            return -EAGAIN;
        log_debug("Empty mailslots: going to sleep");
        if (wait_event_interruptible(slot->inq, (!kfifo_is_empty(&slot->fifo)) ))
            return -ERESTARTSYS; // signal: tell the fs layer to handle it
        // otherwise loop, but first reacquire the lock
        if(mutex_lock_interruptible(&slot->r_lock))
            return -ERESTARTSYS;
    }

    if( kfifo_peek_len(&slot->fifo) > len){
        mutex_unlock(&slot->r_lock);
        log_err("Cannot read mail, destination buffer too short");
        return -EIO;
    }

    ret = kfifo_to_user(&slot->fifo, buf, len, copied);

    mutex_unlock(&slot->r_lock);
    /** End critical section **/

    if(ret){
        log_err("Error while copying to user buffer");
        return ret;
    }
    log_info("Copied %d byte to user buffer", *copied);
    return 0;
}
