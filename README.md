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

More updates soon...

<!---
### To run experiments, varying parameters

Set the enivornmental variables and run the benchmark script
```
  $ source scripts/setvars.sh
  $ python scripts/benchmark.py
```

The benchmark reads the configuration from "input.xml" which 
specifies different experimental configuration and parameters
For example, consider the test that varies the value size of the 
key-value pair. The XML configuration below describes the XML tags.

```
<value-size-main enable='1'> <!-- experiemnt to vary the value size for the dbbench-->
  <seed-count>1</seed-count>  <!-- factor to increment value size in bytes -->
  <num-tests>2</num-tests> <!-- number of tests to run -->
  <memory-levels>2</memory-levels> <!-- NoveLSM uses memtable levels, always set to 2-->
  <thread-count>1</thread-count>  <!-- Number of DBbench's client threads -->
  <value-size>4096</value-size> <!-- Default value size -->
  <num-elements>100000</num-elements> <!-- #. of elements; decremented by seed-count -->
  <DRAM-mem>64</DRAM-mem> <!-- DRAM memtable size in MBs-->
  <NVM-mem>2048</NVM-mem> <!-- NVM memtable size in MBs; atleast 2x larger than DRAM -->
  <num-readthreads>1</num-readthreads> <!-- Read parallelism threads (use 0 or 1 for now) -->
</value-size-main>


<benchmarks> <!-- tags specify type of DBbench's benchmark -->
  <test>fillrandom</test>
  <test>readrandom</test>
</benchmarks>
```
-->

