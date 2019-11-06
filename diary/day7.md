Creating HDD.

source: https://wiki.archlinux.org/index.php/QEMU
1) Create a formatted disk image
qemu-img create -f raw hdd.img 2G

2) Connect it to QEMU
qemu-system-x86_64 -cdrom kernel.iso -serial stdio -m 1024M -drive file=hdd.img,format=raw

(Note you can specify boot order)
qemu-system-x86_64 -cdrom iso_image -boot order=d -drive file=disk_image,format=raw

3) mkfs.ext2 hdd.img

4) sudo mount -t ext2 -o loop hdd.img /mnt

Copy current fs to mnt. This will make it to write hdd.img
5) cp -a fs/. /mnt/.

6) Use debugfs to check the directory is written in hdd.img
