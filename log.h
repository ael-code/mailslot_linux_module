#ifndef __LOG_H__
#define __LOG_H__

#define LOG(lvl, msg, ...) printk(lvl "%s: "msg"\n", KBUILD_MODNAME, ##__VA_ARGS__)
#define log_debug(msg, ...) LOG(KERN_DEBUG, msg, ##__VA_ARGS__)
#define log_err(msg, ...) LOG(KERN_ERR, msg, ##__VA_ARGS__)
#define log_info(msg, ...) LOG(KERN_INFO, msg, ##__VA_ARGS__)

#endif
