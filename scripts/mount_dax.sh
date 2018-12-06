#!/bin/bash -x
#script to create and mount a pmemdir
#requires size as input
sudo umount $TEST_TMPDIR
sudo mkdir $TEST_TMPDIR
sudo mkfs.ext4 /dev/pmem0
sudo mount dax /dev/pmem0 $TEST_TMPDIR
sudo chown -R $USER $TEST_TMPDIR
