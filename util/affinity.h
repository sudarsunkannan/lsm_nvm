#include <pthread.h>
#include <sched.h>

int setaffinity(int cpuid)
{
        int s;
        cpu_set_t cpuset;
        pthread_t thread;

        thread = pthread_self();
        CPU_ZERO(&cpuset);
        CPU_SET(cpuid, &cpuset);
        s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
        if (s != 0)
	  return -1;
        return 0;
}

int setaffinity_arr(int *arr, int count, int index)
{
        int s = 0, i = 0;
        cpu_set_t cpuset;
        pthread_t thread;

        thread = pthread_self();
        CPU_ZERO(&cpuset);

    	for (i = 0; i < count; i++)
    	    CPU_SET(arr[index+i], &cpuset);

        s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
        if (s != 0)
	    return -1;

        return 0;
}

int getaffinity()
{
	int s, j;
	cpu_set_t cpuset;
	pthread_t thread;

	thread = pthread_self();

	/* Set affinity mask to include CPUs 0 to 7 */
	CPU_ZERO(&cpuset);
	//for (j = 0; j < 8; j++)
	//CPU_SET(j, &cpuset);
	//s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
	//if (s != 0)
	//handle_error_en(s, "pthread_setaffinity_np");

	/* Check the actual affinity mask assigned to the thread */
	s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
	if (s != 0)
	    perror("pthread_getaffinity_np");

	//printf("Set returned by pthread_getaffinity_np() contained:\n");
	for (j = 0; j < CPU_SETSIZE; j++)
	if (CPU_ISSET(j, &cpuset))
	   printf("    CPU %d\n", j);
   
	return 0;
}
