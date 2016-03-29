#ifndef __SLOTS_H__
#define __SLOTS_H__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kfifo.h>
#include <linux/kdev_t.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/wait.h>

#include "log.h"

#define MAX_SLOTS ((1 << MINORBITS) -1)
//2 char
#define RECORD_SIZE ((1 << 16) -1)

typedef struct kfifo_rec_ptr_2 kfifo_rec;

struct slot{
    struct mutex r_lock;
    struct mutex w_lock;
    wait_queue_head_t inq;
    kfifo_rec fifo;
};

typedef struct slot slot_t;

int slot_init(slot_t*, size_t);

void slot_free(slot_t*);

int slot_initialized(slot_t*);

int slot_from_user(slot_t*, const void __user *, size_t, unsigned int *);

int slot_to_user(slot_t*, void __user *, size_t, unsigned int *, unsigned short);

#endif
