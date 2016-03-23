#include <linux/kernel.h>
#include <linux/module.h>
#include "log.h"
#include "slots.h"

static int mailslot_open(struct inode *, struct file *);
static ssize_t mailslot_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t mailslot_write(struct file *, const char __user *, size_t, loff_t *);
