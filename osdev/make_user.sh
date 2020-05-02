#!/bin/zsh
cd user; make;cd ..;
sudo cp user/a.out /mnt/.
sync -f /mnt

