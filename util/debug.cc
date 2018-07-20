#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <pthread.h>

//#define DEBUG

//Trouble shooting debug
//use only if trouble shooting
//some specific functions
void DEBUG_T(const char* format, ... ) {
#ifdef DEBUG
		FILE *fp;
        va_list args;
        va_start( args, format );
        vfprintf( stdout, format, args );
        va_end( args );
#endif
}
