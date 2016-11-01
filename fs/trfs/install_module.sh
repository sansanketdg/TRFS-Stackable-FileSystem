#!/bin/sh
set -x
# WARNING: this script doesn't check for errors, so you have to enhance it in case any of the commands
# below fail.
dmesg --cl
rm -f /usr/src/logfile.txt
lsmod
rmmod trfs
insmod trfs.ko
lsmod
