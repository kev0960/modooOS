#!/bin/zsh
cd user; make;cd ..;
sudo cp user/* /mnt/.
sync -f /mnt

