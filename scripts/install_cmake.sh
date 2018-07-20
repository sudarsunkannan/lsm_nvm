#!/bin/bash
git clone https://github.com/Kitware/CMake.git
cd CMake
./configure
make -j16
sudo make install
cd $NOVELSMSRC
