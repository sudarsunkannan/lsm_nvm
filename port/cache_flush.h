#ifndef CACHE_FLUSH_H
#define CACHE_FLUSH_H

#include <stdio.h>
#include <stdlib.h>

#ifdef _ENABLE_PMEMIO
#include "pmdk/src/include/libpmem.h"
#endif

//Cacheline size
//TODO: Make it configurable
#define CACHE_LINE_SIZE 64
#define ASMFLUSH(dest) __asm__ __volatile__ ("clflush %0" : : "m"(*(volatile char *)dest))

static inline void clflush(volatile char* __p)
{
    asm volatile("clflush %0" : "+m" (*__p));
}

static inline void mfence()
{
    asm volatile("mfence":::"memory");
    return;
}

static inline void flush_cache(void *ptr, size_t size){

#ifdef _ENABLE_PMEMIO
  pmem_persist((const void*)ptr, size);
#else
  unsigned int  i=0;
  uint64_t addr = (uint64_t)ptr;	

  mfence();
  for (i =0; i < size; i=i+CACHE_LINE_SIZE) {
	clflush((volatile char*)addr);
	addr += CACHE_LINE_SIZE;
  }
  mfence();
#endif
}

static inline void memcpy_persist
                    (void *dest, void *src, size_t size){

#ifdef _ENABLE_PMEMIO
  pmem_memcpy_persist(dest, (const void *)src, size);
#else
  unsigned int  i=0;
  uint64_t addr = (uint64_t)dest;
  memcpy(dest, src, size);

  mfence();
  for (i =0; i < size; i=i+CACHE_LINE_SIZE) {
    clflush((volatile char*)addr);
    addr += CACHE_LINE_SIZE;
  }
  mfence();
#endif

}
#endif
