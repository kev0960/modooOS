#!/bin/zsh
qemu-img create -f raw hdd.img 2G
mkfs.ext2 -b 1024 -I 128 hdd.img
