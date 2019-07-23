#!/bin/zsh
cd boot; make;cp kernel ../iso/boot; cd ..
grub-mkrescue -o kernel.iso iso
qemu-system-x86_64 -cdrom kernel.iso -serial stdio -m 1024M
