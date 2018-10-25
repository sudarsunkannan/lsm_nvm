## NoveLSM
NoveLSM is a persistent-memory implementation of LevelDB. More details are in ATC '18 paper. Note, this is a beta version and we are making more updates (see below). Pleaseadd problems/questions to the issues tab and we will try our best to fix them soon. Ofcourse your patches are most welcome :)

### Compiling NoveLSM
```
  $ cd hoard
  $ ./compile_install_hoard.sh
  $ cd ..
  $ make -j8
```

### Setting up in-memory file system for database
Mount the NVM file system. Please use a large size (e.g., 32GB)
alternatively, reduce key-value entries.
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

### Testing restarts

The script first simply runs the benchmark with random writes for 1M keys and 
reads them during restart. Next, the benchmark randomly kills "random write" operation 
after 5 seconds, restarts, and then reads the data. Look inside the script for 
more details.

```
  $ scripts/run_restart.sh
```

### Running Vanilla LevelDB 
Mainly for performance comparison. NoveLSM is built over LevelDB 1.21</br>
The script compiles the LevelDB 1.21 source code with release version and uses NVM for SSTable</br>
Disable Snappy compression; If enabled, Snappy will only compress SSTable. </br>
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

### For using Intel's PMDK APIs

This beta includes Intel's PMDK persistent memory copy and flush
functionalities. <br> To use Intel's PMDK APIs instead of the default NoveLSM API 
use the following instruction. 

```
  $ vim build_detect_platform 
 
  //Enable PMEMIO flag
  COMMON_FLAGS="$COMMON_FLAGS -D_ENABLE_PMEMIO"   

  //follow instructions from https://github.com/pmem/pmdk 
  //for dependencies
  $ scripts/install_pmemio.sh

  $ source scripts/setvars.sh
  $ scripts/run_dbbench.sh
```
### Ongoing Fixes
We are currently getting rid of PMDK's transaction dependence (due to high overhead) and frequently changing interfaces and implementing our own NVM transactions tailored for LSMs. Current code provides simple persistent commits.

We are also changing our thread pool implementation that avoids core-sharing across threads. Hence, we use only one additional thread dedicated to memtable searches in addition to the main process threads. Also note that, as noted in the paper, performance of read threading is dependent on the object size and size of the db.


