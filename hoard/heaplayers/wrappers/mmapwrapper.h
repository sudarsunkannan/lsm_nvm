// -*- C++ -*-

/*

  Heap Layers: An Extensible Memory Allocation Infrastructure
  
  Copyright (C) 2000-2012 by Emery Berger
  http://www.cs.umass.edu/~emery
  emery@cs.umass.edu
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef HL_MMAPWRAPPER_H
#define HL_MMAPWRAPPER_H

#if defined(_WIN32)
#include <windows.h>
#else
// UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <map>
#endif
#include <assert.h>
#include <numa.h>



static size_t total_alloc=0;
static unsigned long mapfilesz=4294967296;
static int first_time=1;
static int gfd = 0;
static int numanode=1;

//#define MAP_LARGE_MMAP
#ifdef MAP_LARGE_MMAP
#define MAX 1024*1024*1024*1
static void *g_ptr=NULL;
static void *g_currptr=NULL;
#endif


/*Different types of MMAP we support*/
//#define MAP_DRAM
#ifdef MAP_DRAM
static int mapFlag = MAP_PRIVATE | MAP_ANONYMOUS;
#endif

//#define MAP_MNEMOSYNE
#ifdef MAP_MNEMOSYNE
#define MAP_SCM 0x80000
#define MAP_PERSISTENT MAP_SCM
static int mapFlag = MAP_PRIVATE;
#endif

#define MAP_PMFS
#ifdef MAP_PMFS
static int mapFlag = MAP_PRIVATE;
#define PMFSPATH "/mnt/ramdisk"
#endif

//#define MAP_PVMAP
#ifdef MAP_PVMAP
#include <time.h>
#define __NR_nv_mmap_pgoff     314
#define _USERANDOM_PROCID
struct nvmap_arg_struct {

    unsigned long fd;
    unsigned long offset;
    int vma_id;
    int proc_id;
    /*flags related to persistent memory usage*/
    int pflags;
    int noPersist; // indicates if this mmapobj is persistent or not
    int ref_count;
};
#define BASEPROCID 347
typedef struct nvmap_arg_struct nvarg_s;
static int mapFlag = MAP_PRIVATE | MAP_ANONYMOUS;
#endif


#if defined(__SVR4)
// Not sure why this is needed for Solaris, but there it is.
extern "C" int madvise (caddr_t, size_t, int);
#endif

#if !defined(HL_MMAP_PROTECTION_MASK)
#if HL_EXECUTABLE_HEAP
#define HL_MMAP_PROTECTION_MASK (PROT_READ | PROT_WRITE | PROT_EXEC)
#else
#define HL_MMAP_PROTECTION_MASK (PROT_READ | PROT_WRITE)
#endif
#endif

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif



namespace HL {

  class MmapWrapper {
  public:

#if defined(_WIN32) 
  
    // Microsoft Windows has 4K pages aligned to a 64K boundary.
    enum { Size = 4 * 1024 };
    enum { Alignment = 64 * 1024 };

#elif defined(__SVR4)
    // Solaris aligns 8K pages to a 64K boundary.
    enum { Size = 8 * 1024 };
    enum { Alignment = 64 * 1024 };

#else
    // Linux and most other operating systems align memory to a 4K boundary.
    enum { Size = 4 * 1024 };
    enum { Alignment = 4 * 1024 };

#endif

#ifdef MAP_LARGE_MMAP
static size_t get_large_mmap_range(void *start, void *end){

    size_t diff;
    *(unsigned long *)start= (unsigned long)g_ptr;
    *(unsigned long *)end=(unsigned long)g_ptr + mapfilesz;
    diff=*(unsigned long *)end -  *(unsigned long *)start;
    //fprintf(stdout,"map diff %zu \n",diff);	
    return diff;
}
#endif


    // Release the given range of memory to the OS (without unmapping it).
    void release (void * ptr, size_t sz) {
      if ((size_t) ptr % Alignment == 0) {
	// Extra sanity check in case the superheap's declared alignment is wrong!
#if defined(_WIN32)
	VirtualAlloc (ptr, sz, MEM_RESET, PAGE_NOACCESS);
#elif defined(__APPLE__)
	madvise (ptr, sz, MADV_DONTNEED);
	madvise (ptr, sz, MADV_FREE);
#else
	// Assume Unix platform.
	madvise ((caddr_t) ptr, sz, MADV_DONTNEED);
#endif
      }
    }

#if defined(_WIN32) 
  
    static void protect (void * ptr, size_t sz) {
      DWORD oldProtection;
      VirtualProtect (ptr, sz, PAGE_NOACCESS, &oldProtection);
    }
    
    static void unprotect (void * ptr, size_t sz) {
      DWORD oldProtection;
      VirtualProtect (ptr, sz, PAGE_READWRITE, &oldProtection);
    }

    static void * map (size_t sz) {
      void * ptr;
#if HL_EXECUTABLE_HEAP
      const int permflags = PAGE_EXECUTE_READWRITE;
#else
      const int permflags = PAGE_READWRITE;
#endif
      ptr = VirtualAlloc (NULL, sz, MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN, permflags);
      return  ptr;
    }
  
    static void unmap (void * ptr, size_t) {
      VirtualFree (ptr, 0, MEM_RELEASE);
    }

#else // UNIX

    static void protect (void * ptr, size_t sz) {
      mprotect ((char *) ptr, sz, PROT_NONE);
    }
    
    static void unprotect (void * ptr, size_t sz) {
      mprotect ((char *) ptr, sz, PROT_READ | PROT_WRITE | PROT_EXEC);
    }

	
static int util_tmpfile(const char *dir, size_t size)
{
		int errno;
        static char temp[] = "/vmem.XXXXXX";
        char fullname[strlen(dir) + sizeof (temp)];
        (void) strcpy(fullname, dir);
        (void) strcat(fullname, temp);
        int fd;
        if ((fd = mkstemp(fullname)) < 0) {
                printf("!mkstemp");
		assert(0);
                goto err;
        }
        (void) unlink(fullname);

		if(ftruncate(fd, size)==0) {
			return fd;
		}else {
			printf("!ftruncate fname %s, "
				"fd %d, size %zu\n", fullname,fd, size);
		}
        if ((errno = posix_fallocate(fd, 0, size)) != 0) {
                printf("!posix_fallocate");
		assert(0);
                goto err;
        }
        return fd;
err:
        if (fd != -1)
                (void) close(fd);
        return -1;
}

static void * map (size_t sz) {

      void * ptr;
      char * startAddress = 0;

      if (sz == 0) {
		return NULL;
      }
      // Round up the size to a page-sized value.
      sz = Size * ((sz + Size - 1) / Size);
#if defined(MAP_ALIGN) && defined(MAP_ANON)
      int fd = -1;
      startAddress = (char *) Alignment;
      mapFlag |= MAP_PRIVATE | MAP_ALIGN | MAP_ANON;
#elif !defined(MAP_ANONYMOUS)
      static int fd = ::open ("/dev/zero", O_RDWR);
      mapFlag |= MAP_PRIVATE;
#else
      int fd = -1;
      mapFlag |= MAP_ANONYMOUS | MAP_PRIVATE;
#endif

#if defined(MAP_PMFS)
		if(first_time){
 			fprintf(stderr,"calling pmfs mmap %ld\n", mapfilesz);
 			gfd = util_tmpfile(PMFSPATH, mapfilesz);

 		 	if(gfd < 0){
				fprintf(stderr,"mmap failed \n");
				exit(-1);	
 			}	
	 		first_time=0;
		}
		fd = gfd;
		ptr = mmap (startAddress, sz, HL_MMAP_PROTECTION_MASK,  mapFlag, fd, 0);
		total_alloc += sz;
#elif !defined(MAP_PVMAP) && defined(MAP_LARGE_MMAP)
	    if(!g_ptr || ((g_currptr+sz) > g_ptr + mapfilesz)) {
      	  //g_ptr = mmap (0,mapfilesz, HL_MMAP_PROTECTION_MASK, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
          g_ptr = numa_alloc_onnode(mapfilesz, numanode);
          fprintf(stdout,"mmaping %lu\n", mapfilesz);
    	  assert(g_ptr != MAP_FAILED);
          assert(g_ptr != NULL);
	      g_currptr = g_ptr;
	    }
		ptr = g_currptr;
		g_currptr += sz;


#elif !defined(MAP_PVMAP)
		//fprintf(stderr,"calling mmap %u\n", sz);	
		startAddress = 0;
        //ptr = numa_alloc_onnode(sz, numanode);
		ptr = mmap (startAddress, sz, HL_MMAP_PROTECTION_MASK,  mapFlag, fd, 0);
#else
		nvarg_s nvarg;
		nvarg.pflags = 1;
		nvarg.noPersist = 1;
		nvarg.vma_id = 9998;
		nvarg.proc_id = BASEPROCID;
		//fprintf(stderr,"using NVMMAP %u\n", sz);
 		ptr = (void *)syscall(__NR_nv_mmap_pgoff,startAddress, sz, HL_MMAP_PROTECTION_MASK, MAP_PRIVATE | MAP_ANONYMOUS, &nvarg);
#if 0
		if(!first_time){
		fprintf(stderr,"not first time \n");
		syscall(__NR_nv_mmap_pgoff,startAddress, 4096, HL_MMAP_PROTECTION_MASK, MAP_PRIVATE | MAP_ANONYMOUS, &nvarg);
		first_time=1;
			fprintf(stdout,"not first time \n");		
		}
		ptr = mmap (startAddress, sz, HL_MMAP_PROTECTION_MASK,  mapFlag, fd, 0);
#endif
#endif
      if (ptr == MAP_FAILED) {
				return NULL;
      } else {
				return ptr;
      }
    }

    static void unmap (void * ptr, size_t sz) {
      // Round up the size to a page-sized value.
      sz = Size * ((sz + Size - 1) / Size);
      total_alloc -= sz;	
      munmap ((caddr_t) ptr, sz);
    }
#endif
  };
}
#endif
