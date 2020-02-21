#!/bin/zsh
cd kernel; make -j 8;cp kernel.bin ../iso/boot/kernel; cd ..
grub-mkrescue -o kernel.iso iso
qemu-system-x86_64 -cdrom kernel.iso -boot order=d -serial stdio -m 2048M -drive file=hdd.img,format=raw
