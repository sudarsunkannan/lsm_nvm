#!/bin/bash -x
#script to create and mount a pmemdir
#requires size as input
sudo umount /mnt/pmemdir

if [[ x$1 == x ]];
   then
      echo You have specify correct pmemdir size in GB
      exit 1
   fi

sudo mkdir /mnt/pmemdir
sudo chown -R $USER /mnt/pmemdir
chmod 777 /mnt/pmemdir
sudo mount -t tmpfs -o size=$1M tmpfs /mnt/pmemdir

