#include <stdio.h>

int print_numa(void);
int get_numa_count(void);
int get_node_cpus(int node);
int get_num_cpus();
int* get_used_cpu_map();
int* get_ftlcpu_map();
int fill_cpumap_info(void);
int get_free_core(void);
