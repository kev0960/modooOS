#!/bin/zsh
cd boot; make;cp kernel ../iso/boot; cd ..
grub-mkrescue -o kernel.iso iso
qemu-system-x86_64 -cdrom kernel.iso -boot order=d -serial stdio -m 1024M -drive file=hdd.img,format=raw
