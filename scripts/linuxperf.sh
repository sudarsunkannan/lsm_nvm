#/users/skannan/ssd/schedsp/linux-4.5.4/tools/perf/perf stat -e instructions -e LLC-store-misses -e LLC-load-misses $1

export PREFIX="/users/skannan/ssd/schedsp/linux-4.5.4/tools/perf/perf stat -e instructions -e LLC-store-misses -e LLC-load-misses"
$1

#/usr/bin/time -v $1
#/opt/intel/vtune_amplifier_xe_2013/bin64/amplxe-cl -collect-with runsa -knob event-config=L2_LINES_IN.SELF.ANY,DATA_TLB_MISSES.DTLB_MISS,INST_RETIRED.ANY $1  

#LD_PRELOAD=/usr/lib/libhoard.so $1

#echo "Enter application name and arguments as params for the script"
#sudo chmod o+rw /dev/cpu/*/msr
#sudo modprobe msr
#sudo likwid-perfctr  -C 0-3 -g INSTR_RETIRED_ANY:FIXC0,DTLB_LOAD_MISSES_ANY:PMC0,L2_LINES_IN_ANY:PMC1 $1
#sudo likwid-perfctr  -C 0-7  -g INSTR_RETIRED_ANY:FIXC0,MEM_UNCORE_RETIRED_LOCAL_DRAM:PMC0,MEM_UNCORE_RETIRED_REMOTE_DRAM:PMC1 $1
#sudo likwid-perfctr  -C 0-7 -g INSTR_RETIRED_ANY:FIXC0,MEM_UNCORE_RETIRED_LOCAL_DRAM:PMC0,MEM_INST_RETIRED_LOADS:PMC1,MEM_INST_RETIRED_STORES:PMC2,UNC_L3_MISS_ANY:UPMC0 $1
#sudo likwid-perfctr  -C 0-7 -g INSTR_RETIRED_ANY:FIXC0 $1
#sudo likwid-powermeter -p  $1
#/usr/bin/time -v $1
