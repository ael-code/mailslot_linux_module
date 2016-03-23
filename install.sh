#!/bin/bash

grep "mailslots" /proc/modules
[ $? -eq 0 ] && make unregister
make register
