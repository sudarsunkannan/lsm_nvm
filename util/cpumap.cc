#include <numa.h>
#include "bitops.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sched.h>

static int maxnode;
int cpumap[MAXCORES];
int used_cpu_map[MAXCORES];
int g_ncpus;


int print_numa(void)
{
    int i, k, w, ncpus;
    struct bitmask *cpus;
    maxnode = numa_num_configured_nodes()-1;

    if (numa_available() < 0)  {
        printf("no numa\n");
        exit(1);
    }
    cpus = numa_allocate_cpumask();
    ncpus = cpus->size;

    for (i = 0; i <= maxnode ; i++) {
    if (numa_node_to_cpus(i, cpus) < 0) {
        printf("node %d failed to convert\n",i);
    }
    printf("%d: ", i);
    w = 0;
    for (k = 0; k < ncpus; k++)
        if (numa_bitmask_isbitset(cpus, k))
            printf(" %s%d", w>0?",":"", k);
    putchar('\n');
    }
    return 0;
}

int get_numa_count(void) {
    maxnode = numa_num_configured_nodes();      
    return maxnode;
}


int get_num_cpus() {

    struct bitmask *cpus;
    int ncpus;
    
    cpus = numa_allocate_cpumask();
    ncpus = cpus->size;
    return ncpus;
}


//Get the cores to be used for host CPU thread;
int* get_used_cpu_map() {
    return cpumap;
}


//Get the cores to be used for FTL CPU thread;
int* get_ftlcpu_map() {
    return used_cpu_map;
}


int get_free_core(void) {

    int i = 0;

    for (i = g_ncpus-1; i >= 0; i--){
        if(used_cpu_map[i] == 0) {
	    used_cpu_map[i] = 1;
	    return cpumap[i];	
	}
    }
    return -1;	
}


/*CPUs that are not currently used by 
current process*/
int fill_used_cpu_map(int node, int numcpus) {

    int i, k, w, ncpus;
    struct bitmask *cpus;

    pid_t pid = getpid();
    cpu_set_t cpuset;
    int ret;

    CPU_ZERO(&cpuset);

    //TODO: Take care if this fails
    if(sched_getaffinity(0, sizeof(cpuset), &cpuset)) {
	perror("get_free_cpus failed \n");
	return -1;
    }
    
    for (i = 0; i < numcpus; ++i){
        //if (CPU_ISSET(i, &cpuset)) {
	  //  used_cpu_map[i] = 1;
        //}
        fprintf(stderr, "CPU map %d \n", cpumap[i]);
    }
    return i;
}


/* Get all this CPUs in this node 
* Fill the CPU ID in cpumap 
* Increment global CPU count and return 
*/
int get_node_cpus(int node) {

    int i, k, w, ncpus = 0;
    struct bitmask *cpus;

    g_ncpus = 0; 	
    //maxnode = numa_num_configured_nodes()-1;
    if (numa_available() < 0)  {
        printf("no numa\n");
        return -1;
    }
    cpus = numa_allocate_cpumask();
    ncpus = cpus->size;
    
    //for(i=node; i<= node; i++) {
    if (numa_node_to_cpus(node, cpus) < 0) {
        printf("node %d failed to convert\n",i);
    }

    w = 0; i = 0;
    for (k = 1; k < ncpus; k++)
        if (numa_bitmask_isbitset(cpus, k)) {
            cpumap[g_ncpus] = k;
	    //fprintf(stderr, "cpumap[%d]:%d: ", i, k);	
            used_cpu_map[g_ncpus] = 0;
	    g_ncpus++;
        }
    //}
    numa_free_cpumask(cpus);    
    return g_ncpus;
}


int fill_cpumap_info(void) {

    //Get the number of NUMA nodes
    int numacnt = get_numa_count();
    int numcpus = -1, nodenum = 0;
    //Get the node affinity
    char *node = getenv("NUMA_AFFINITY");
    //Nothing critical. Just set the node to 0	
    if(!node) {
        nodenum = 0;	
    }else {
        nodenum = atoi(node);
    }

    if(numacnt < 1) {
        perror("NUMA node enabled. Install NUMA lib \n");
        return -1;
    }

    if ( (numcpus = get_node_cpus(nodenum)) < 1) {
        printf("failed to fill CPUS %d\n", numcpus);
        return -1;
    }
    //fill_used_cpu_map(nodenum, numcpus);
    return 0;
}




