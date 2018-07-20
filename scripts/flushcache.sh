#!/bin/bash
sudo sh -c "echo 3 > /proc/sys/vm/drop_caches"
sudo sh -c "sync"
sudo sh -c "sync"
sudo sh -c "echo 3 > /proc/sys/vm/drop_caches"

rm -rf /mnt/pmemdir/chk*
rm -rf /mnt/pmemdir/dbbench/*
