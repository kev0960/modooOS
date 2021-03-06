#!/bin/zsh
cd kernel; make -j 8;cp kernel.bin ../iso/boot/kernel; cd ..
grub-mkrescue -o kernel.iso iso
qemu-system-x86_64 -cdrom kernel.iso -boot order=d -monitor stdio -m 2G -drive file=hdd.img,format=raw -smp 4 -serial file:serial.log
