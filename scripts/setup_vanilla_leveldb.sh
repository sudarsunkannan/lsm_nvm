#!/bin/bash
set -x
export TEST_TMPDIR="/mnt/pmemdir"

SETUP() {
    cd $NOVELSMSRC
    #git clone https://github.com/google/leveldb
    cd $NOVELSMSRC/leveldb-1.20
}

MAKE() {
  #mkdir out-static
  #cd out-static
  #rm -rf CMakeCache.txt
  #cmake -DCMAKE_BUILD_TYPE=Release ..
  make clean
  make -j$PARA
  cd $NOVELSMSRC
}

SETUP
MAKE
