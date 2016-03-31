#ifndef __MAILSLOT_IOCTL_H__
#define __MAILSLOT_IOCTL_H__

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

#define MAILSLOT_IOC_MAGIC 'l'
#define __START_CMD_NUM 128
#define DEFAULT_SLOT_SIZE 1024

#define MAILSLOT_IOC_SET_NONBLOCKING _IOW(MAILSLOT_IOC_MAGIC, __START_CMD_NUM + 0, unsigned int)
#define MAILSLOT_IOC_IS_NONBLOCKING _IOR(MAILSLOT_IOC_MAGIC, __START_CMD_NUM + 1, unsigned int)
#define MAILSLOT_IOC_RESIZE _IOR(MAILSLOT_IOC_MAGIC, __START_CMD_NUM + 2, unsigned int)

#endif
