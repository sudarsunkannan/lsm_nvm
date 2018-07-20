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
#include <assert.h>
#endif
#include <numa.h>

#if HL_EXECUTABLE_HEAP
#define HL_MMAP_PROTECTION_MASK (PROT_READ | PROT_WRITE | PROT_EXEC)
#else
#define HL_MMAP_PROTECTION_MASK (PROT_READ | PROT_WRITE)
#endif

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif


#define SINGLE_LARGE_MMAP
#ifdef SINGLE_LARGE_MMAP
#define MAX 4294967296
static void *g_ptr=NULL;
static void *g_currptr=NULL;
#endif

//#define _USE_NVMAP
#ifdef _USE_NVMAP
#include <time.h> 
static unsigned int BASEPROCID=0;
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
typedef struct nvmap_arg_struct nvarg_s;

static size_t nvm_tot_allocs;
static void nvm_stat_print();
static void nvm_add_alloc_stat(size_t sz);
static void nvm_add_free_stat(size_t sz);
size_t get_large_mmap_range(void *start, void *end);

static void nvm_add_alloc_stat(size_t sz){
    nvm_tot_allocs += sz;
}
static void nvm_add_free_stat(size_t sz){
    nvm_tot_allocs -= sz;
}

static void nvm_stat_print(){
    fprintf(stderr,"total active allocs %zu \n",nvm_tot_allocs);
}
#endif


#ifdef SINGLE_LARGE_MMAP
size_t get_large_mmap_range(void *start, void *end){
	
    size_t diff;

    *(unsigned long *)start= (unsigned long)g_ptr;
    *(unsigned long *)end=(unsigned long)g_currptr;
    diff=*(unsigned long *)end -  *(unsigned long *)start;
    return diff;	          
}
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

#endif

static unsigned int pagesize;
static unsigned int page_count, numpages;
static size_t mapsize;
static int offset;
static void **addr;
static int init;

static void * map (size_t sz) {

      if (sz == 0) {
		return NULL;
      }
      // Round up the size to a page-sized value.
      sz = Size * ((sz + Size - 1) / Size);

      void * ptr;
#if defined(MAP_ALIGN) && defined(MAP_ANON)
      // Request memory aligned to the Alignment value above.
      ptr = mmap ((char *) Alignment, sz, HL_MMAP_PROTECTION_MASK, MAP_PRIVATE | MAP_ALIGN | MAP_ANON, -1, 0);
      fprintf(stdout,"mmap 3\n");
#elif !defined(MAP_ANONYMOUS)
      static int fd = ::open ("/dev/zero", O_RDWR);
      ptr = mmap (NULL, sz, HL_MMAP_PROTECTION_MASK, MAP_PRIVATE, fd, 0);
      fprintf(stdout,"mmap 2\n");	
#else

 
#ifndef _USE_NVMAP
 #ifdef SINGLE_LARGE_MMAP
     if(!g_ptr || ((g_currptr+sz) > g_ptr + MAX)) {
		 //fprintf(stdout,"mmaping \n");
         g_ptr = mmap (0,MAX, HL_MMAP_PROTECTION_MASK, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		 assert(g_ptr != MAP_FAILED);
		 assert(g_ptr != NULL);
		 g_currptr = g_ptr;
	}
	ptr = g_currptr;
	g_currptr += sz;
	
 #else
      //fprintf(stdout,"calling mmap HL_MMAP_PROTECTION_MASK \n");	
      ptr = mmap (0, sz, HL_MMAP_PROTECTION_MASK, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
 #endif // #ifndef _USE_NVMAP

#else //_USE_NVMAP
	if(!BASEPROCID){
#ifdef _USERANDOM_PROCID
      struct timeval currtime;
	  int iSecret;
      /* initialize random seed: */
	  srand (time(NULL));
	  /* generate secret number between 1 and 10: */
      gettimeofday(&currtime, NULL);
	  iSecret = (rand() % currtime.tv_usec % 533333 ) + 1;
      BASEPROCID= iSecret;
#else
      BASEPROCID = 600;
#endif
	  printf("BASEPROCID %u\n",BASEPROCID);
	}
	 nvarg_s nvarg;
	 nvarg.pflags = 1;
	 nvarg.noPersist = 1;
	 nvarg.vma_id = 9998;
	 nvarg.proc_id = BASEPROCID;


#ifdef SINGLE_LARGE_MMAP
     if(!g_ptr || ((g_currptr+sz) > g_ptr + MAX)) {
		 fprintf(stdout,"mmaping \n");
		 g_ptr = (void *)syscall(__NR_nv_mmap_pgoff,0, MAX, HL_MMAP_PROTECTION_MASK, MAP_PRIVATE | MAP_ANONYMOUS, &nvarg);
		 assert(g_ptr != MAP_FAILED);
		 assert(g_ptr != NULL);
		 g_currptr = g_ptr;
		 unsigned int i=0;
		 char *temp = (char *)g_currptr;
		 char a;
		 while(i<MAX){	
			temp[i]++;
			temp[i]=0;
			i++;
		}	
		fprintf(stdout,"finished mapping \n");
	}	
	ptr = g_currptr;
	g_currptr += sz;

#else
	 //nvm_add_alloc_stat(sz);
	 fprintf(stdout,"using NVMMAP %u\n", sz);
	 ptr = (void *)syscall(__NR_nv_mmap_pgoff,0, sz, HL_MMAP_PROTECTION_MASK, MAP_PRIVATE | MAP_ANONYMOUS, &nvarg);
	 //nvm_stat_print(); 
#endif
     //np_nvmap  = (char *)syscall(__NR_nv_mmap_pgoff,0 ,s,  PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, &a);
#endif
      //fprintf(stdout,"mmap 1\n");	
#endif

      if (ptr == MAP_FAILED) {
	char buf[255];
	sprintf (buf, "Out of memory!");
	fprintf (stderr, "%s\n", buf);
	return NULL;
      } else {
	return ptr;
      }
    }

    static void unmap (void * ptr, size_t sz) {
      // Round up the size to a page-sized value.
      sz = Size * ((sz + Size - 1) / Size);
      munmap ((caddr_t) ptr, sz);
    }
#endif
  };
}

#endif
