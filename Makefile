MODULE_NAME=mailslot
KVERSION = $(shell uname -r)

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y := slots.o

all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean

register:
	insmod $(MODULE_NAME).ko

unregister:
	rmmod $(MODULE_NAME)

test: register unregister

.PHONY: clean register unregister
