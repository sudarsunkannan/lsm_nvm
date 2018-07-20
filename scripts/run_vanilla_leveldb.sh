#!/bin/bash
#set -x
NUMTHREAD=1
BENCHMARKS="fillrandom,readrandom"
NUMKEYS="1000000"
let BUFFBYTES=$DRAMBUFFSZ*1024*1024
OTHERPARAMS="--write_buffer_size=$BUFFBYTES"
VALUSESZ=4096

SETUP() {
  if [ -z "$TEST_TMPDIR" ]
  then
        echo "DB path empty. Run source scripts/setvars.sh from source parent dir"
        exit
  fi
  rm -rf $TEST_TMPDIR/*
  mkdir -p $TEST_TMPDIR
}

MAKE() {
  cd $LEVELDB_VANILLA
  #make clean
  make -j8
}

SETUP
MAKE
$DBBENCH_VANLILLA/db_bench --threads=$NUMTHREAD --num=$NUMKEYS --benchmarks=$BENCHMARKS --value_size=$VALUSESZ $OTHERPARAMS
SETUP

#Run all benchmarks
$DBBENCH_VANLILLA/db_bench --threads=$NUMTHREAD --num=$NUMKEYS --value_size=$VALUSESZ $OTHERPARAMS

