#!/bin/zsh
cd user; make;cd ..;
sudo cp -r user/* /mnt/.
sync -f /mnt

