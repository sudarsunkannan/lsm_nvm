#!/bin/bash -x
#script to create and mount a pmemdir
#requires size as input
sudo umount $TEST_TMPDIR
sudo mkdir $TEST_TMPDIR
sudo modprobe nova
sudo mount -t NOVA -o init /dev/pmem0 $TEST_TMPDIR
sudo chown -R $USER $TEST_TMPDIR
