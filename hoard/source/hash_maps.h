#ifndef HASH_MAPS_H_
#define HASH_MAPS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include "nv_map.h"
#include "nv_def.h"
#include "nv_structs.h"

//int record_addr(void* addr, size_t size);
void* get_chunk_from_map(void *addr);
void* get_chunk_with_id(UINT chunkid);
int record_chunks(void* addr, chunkobj_s *chunk);
int get_chnk_cnt_frm_map();
void *get_chunk_from_map_o1(void *addr);
int record_vmas(int vmaid, size_t size);
int record_metadata_vma(int vmaid, size_t size);
size_t get_vma_size(int vmaid);
void add_alloc_map(void *ptr, size_t size);
size_t get_alloc_size(void *ptr, unsigned long *addr);
void protect_all_chunks();
void *get_alloc_pagemap(unsigned int *count, size_t **ptr);
#ifdef __cplusplus
}
#endif


#endif
