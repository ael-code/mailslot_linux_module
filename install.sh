#!/bin/bash

grep "mailslot" /proc/modules
[ $? -eq 0 ] && make unregister
make register
