#!/bin/bash
#set -x

NUMTHREAD="--threads=1"
NUMKEYS="--num=1000000"
#NoveLSM specific parameters
#NoveLSM uses memtable levels, always set to num_levels 2
#write_buffer_size DRAM memtable size in MBs
#nvm_buffer_size specifies NVM memtable size; set it in few GBs for perfomance;
DBMEM="--db_mem=/mnt/pmemdir/dbbench --db_disk=/mnt/pmemdir/dbbench"
OTHERPARAMS="--num_levels=2 --write_buffer_size=$DRAMBUFFSZ --nvm_buffer_size=$NVMBUFFSZ $DBMEM"
VALUSESZ="--value_size=4096"
WRITE="--benchmarks=fillrandom"
READ="--benchmarks=readrandom"
PARAMS="$NUMTHREAD $NUMKEYS $WRITE $VALUSESZ $OTHERPARAMS"

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
  cd $NOVELSMSRC
  #make clean
  make -j8
  cd $NOVELSMSCRIPT
}

KILL() {
  #Wait for 5 seconds and kill the process
  sleep 5
  echo " "
  echo "Randomly killing benchmark before execution completion..."
  pids=$(pgrep db_bench) && kill -9 $pids
  sleep 5
  echo " "
  echo "Restarting benchmark"
  echo " "
}


RUNBG() {
$APP_PREFIX $DBBENCH/db_bench $PARAMS $WRITE &
}

RUN() {
$APP_PREFIX $DBBENCH/db_bench $PARAMS $WRITE
}


RESTART() {
$APP_PREFIX $DBBENCH/db_bench $PARAMS $READ
}

SETUP
MAKE


#Simply run a write workload, wait for it to finish, and re-read data
echo " "
echo " "
RUN
echo " "
echo "**************************************"
echo "  SIMPLE RESTART WITHOUT FAILURE      "
echo "**************************************"
echo " "
RESTART

#Run as a background task, then kill the benchmark and restart
#TODO: LevelDB (or may be our bug) does not release this lock.
# So we shamelessly delete it. To be fixed.
echo " "
echo " "
RUNBG
KILL
echo " "
echo " "
echo "**************************************"
echo " RESTART WITH FAILURE    "
echo "**************************************"
echo " "
rm /mnt/pmemdir/dbbench/LOCK
RESTART
