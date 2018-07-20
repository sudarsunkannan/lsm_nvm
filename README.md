## NoveLSM instructions 
Note, this is a beta version. More updates and testing to follow shortly.
Please add problems/questions to the issues tab.

### Compiling NoveLSM
```
  $ cd hoard
  $ ./compile_install_hoard.sh
  $ cd ..
  $ make -j8
```

### Setting up in-memory file system for database
Mount the NVM file system. Please use a large size (e.g., 32GB)
or reduce key-value enteries
```
  $ source scripts/setvars.sh
  $ cd scripts

  //Configure and run Linux DAX file system
  // See https://pmem.io/2016/02/22/pm-emulation.html for 
  // instructions
  $ scripts/mount_dax.sh

  //For using simple ramdisk instead of DAX (performance can be a 
    problem)
  $ scripts/mount_ramdisk.sh MEMSZ_IN_MB
```

## Running NoveLSM

### To run a simple benchmark, 
First set the environment variables and then run the benchmark
```
  $ source scripts/setvars.sh
  $ scripts/run_dbbench.sh
```

### Testing restarts or failure and recovery

The script first simply runs the benchmark with random writes for 1M keys and 
reads them during restart. Next, the benchmark randomly kills "random write" operation 
after 5 seconds, restarts, and then reads the data. Look inside the script for 
more details or varying the values.

```
  $ scripts/run_restart.sh
```

### Running Vanilla LevelDB

NoveLSM is built over LevelDB 1.21. The script compiles the LevelDB source code 
with release version.</br> 
Disable Snappy compression; If enabled, Snappy will only compress SSTable.
Also, ensure that cmake 3.9 or greater is enabled.</br>
```
  # If the system's cmake version is less than 3.9
  $ scripts/install_cmake.sh

  $ scripts/setup_vanilla_leveldb.sh
```
Run the vanilla LevelDB benchmark
```
  $ scripts/run_vanilla_leveldb.sh
```

### For using PMEM IO's persistent APIs

This beta includes Intel's PMDK persistent memory copy and flush
functionalities. To use Intel's PMDK APIs instead of the default NoveLSM API,

```
  $ vim build_detect_platform 
 
  //Enable PMEMIO flag
  COMMON_FLAGS="$COMMON_FLAGS -D_ENABLE_PMEMIO"   

  $ scripts/install_pmemio.sh
```
If you notice errors, follow instructions from https://github.com/pmem/pmdk/
to install pmdk

```
  $ source scripts/setvars.sh
  $ scripts/run_dbbench.sh
```

#### Cite

inproceedings {novelsm, <br>
author = {Sudarsun Kannan and Nitish Bhat and Ada Gavrilovska and Andrea Arpaci-Dusseau and Remzi Arpaci-Dusseau},<br>
title = {Redesigning LSMs for Nonvolatile Memory with NoveLSM},<br>
booktitle = {2018 {USENIX} Annual Technical Conference ({USENIX} {ATC} 18)}, <br>
year = {2018}, <br>
address = {Boston, MA}, <br>
publisher = {{USENIX} Association}, <br>
}
