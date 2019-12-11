#!/bin/zsh
cd kernel; make;cp kernel.bin ../iso/boot/kernel; cd ..
grub-mkrescue -o kernel.iso iso
qemu-system-x86_64 -cdrom kernel.iso -boot order=d -serial stdio -m 1024M -drive file=hdd.img,format=raw
