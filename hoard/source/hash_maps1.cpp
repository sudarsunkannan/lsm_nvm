#include "hash_maps.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <map>
#include <signal.h>
#include <queue>
#include <list>
#include <algorithm>
#include <functional>
#include <pthread.h>
#include <sys/time.h>
#include <limits.h>
#include <unordered_map>
#include <sstream>
#include "malloc_hook.h"
#include "nv_map.h"
#include "nv_def.h"
#include "heaplayers/wrappers/gnuwrapper.h"

using namespace std;

std::unordered_map <void *, chunkobj_s *> chunkmap;
std::unordered_map <void *, chunkobj_s *>::iterator chunk_itr;
std::unordered_map<int, chunkobj_s *> id_chunk_map;

#define MAX_ENTRIES 100*1024*1024
#define OBJECT_TRACK_SZ 512*1024

static int init_alloc;
static unsigned int alloc_cnt;
unsigned long *chunk_addr = NULL;
size_t *chunk_sz = NULL;
static unsigned int offset;


/*****************************************************************************/
#if 0

struct entry_s {
	char *key;
	unsigned int value;
	struct entry_s *next;
};
typedef struct entry_s entry_t;
 
struct hashtable_s {
	int size;
	struct entry_s **table;	
};
 
typedef struct hashtable_s hashtable_t;
unsigned long *hash_cache = NULL;
 
 
/* Create a new hashtable. */
hashtable_t *ht_create( int size ) {
 
	hashtable_t *hashtable = NULL;
	int i;
 
	if( size < 1 ) return NULL;
 
	/* Allocate the table itself. */
	if( ( hashtable = malloc( sizeof( hashtable_t ) ) ) == NULL ) {
		return NULL;
	}
 
	/* Allocate pointers to the head nodes. */
	if( ( hashtable->table = malloc( sizeof( entry_t * ) * size ) ) == NULL ) {
		return NULL;
	}
	for( i = 0; i < size; i++ ) {
		hashtable->table[i] = NULL;
	}
 
	hashtable->size = size;
 
	return hashtable;	
}
 
/* Hash a string for a particular hash table. */
int ht_hash( hashtable_t *hashtable, char *key ) {
 
	unsigned long int hashval;
	int i = 0;
 
	/* Convert our string to an integer */
	while( hashval < ULONG_MAX && i < strlen( key ) ) {
		hashval = hashval << 8;
		hashval += key[ i ];
		i++;
	}
 
	return hashval % hashtable->size;
}
 
/* Create a key-value pair. */
entry_t *ht_newpair( char *key, unsigned int value ) {
	entry_t *newpair;
 
	if( ( newpair = malloc( sizeof( entry_t ) ) ) == NULL ) {
		return NULL;
	}
	if( ( newpair->key = strdup( key ) ) == NULL ) {
		return NULL;
	}

	newpair->value = value;
	newpair->next = NULL;
	return newpair;
}
 
/* Insert a key-value pair into a hash table. */
void ht_set( hashtable_t *hashtable, char *key, unsigned int value ) {
	int bin = 0;
	entry_t *newpair = NULL;
	entry_t *next = NULL;
	entry_t *last = NULL;
 
	bin = ht_hash( hashtable, key );
	next = hashtable->table[ bin ];
	hash_cache[alloc_cnt] = bin;
 
#if 1
	while( next != NULL && next->key != NULL && strcmp( key, next->key ) > 0 ) {
		last = next;
		next = next->next;
	}
 
	/* There's already a pair.  Let's replace that string. */
	if( next != NULL && next->key != NULL && strcmp( key, next->key ) == 0 ) {
 
		//free( next->value );
		//next->value = strdup( value );
 		next->value = value;

	/* Nope, could't find it.  Time to grow a pair. */
	} else {
		newpair = ht_newpair( key, value );
 
		/* We're at the start of the linked list in this bin. */
		if( next == hashtable->table[ bin ] ) {
			newpair->next = next;
			hashtable->table[ bin ] = newpair;
	
		/* We're at the end of the linked list in this bin. */
		} else if ( next == NULL ) {
			last->next = newpair;
	
		/* We're in the middle of the list. */
		} else  {
			newpair->next = next;
			last->next = newpair;
		}
	}
#endif
}
 
/* Retrieve a key-value pair from a hash table. */
unsigned int ht_get( hashtable_t *hashtable, char *key ) {
	int bin = 0;
	entry_t *pair;
 
	bin = ht_hash( hashtable, key );
 
	/* Step through the bin, looking for our value. */
	pair = hashtable->table[ bin ];
	while( pair != NULL && pair->key != NULL && strcmp( key, pair->key ) > 0 ) {
		pair = pair->next;
	}
 
	/* Did we actually find anything? */
	if( pair == NULL || pair->key == NULL || strcmp( key, pair->key ) != 0 ) {
		return NULL;
 
	} else {
		return pair->value;
	}
	
}
#endif

/******************************************************************************/


/*void protect_all_chunks(){ 

	std::unordered_map <void *, size_t>::iterator itr;

	for( itr= allocmap.begin(); itr!=allocmap.end(); ++itr){
        void *addr = (void *)(*itr).first;
		set_chunk_protection(addr,(size_t)(*itr).second,PROT_READ);
		fprintf(stderr,"setting protection for all chunks\n");
	}
}*/

void *get_alloc_pagemap(unsigned int *count, size_t **ptr){

	*count = offset;
	*ptr= chunk_sz;

	return (void *)chunk_addr;
}

void init_allocs() {

	chunk_addr =(unsigned long *)mmap (0, sizeof(unsigned long) * MAX_ENTRIES, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	chunk_sz = (size_t *)mmap (0, sizeof(size_t) * MAX_ENTRIES, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	assert(chunk_addr);
	assert(chunk_sz);

}



void* get_chunk_from_map(void *addr) {

	size_t bytes = 0;
	std::map <int, chunkobj_s *>::iterator id_chunk_itr;
	unsigned long ptr = (unsigned long)addr;
    unsigned long start, end;
	
    for( chunk_itr= chunkmap.begin(); chunk_itr!=chunkmap.end(); ++chunk_itr){
        chunkobj_s * chunk = (chunkobj_s *)(*chunk_itr).second;
        bytes = chunk->length;
		start = (ULONG)(*chunk_itr).first;
		end = start + bytes;

		if( ptr >= start && ptr <= end) {
			return (void *)chunk;
		}
	}
	return NULL;
}

chunkobj_s* create_chunk(void *addr, size_t size){

	chunkobj_s *chunk = NULL;

	 chunk = ( chunkobj_s *)xxmalloc(sizeof(chunkobj_s));
	 assert(chunk);	  
	 chunk->nv_ptr = addr;		
	 chunk->length = size;
	 return chunk;
}

int record_chunks(void* addr, chunkobj_s *chunk) {

	chunkmap[addr] = chunk;
	return 0;
}


int record_addr(void* addr, size_t size) {

	//chunkobj_s *chunk;
	//chunk = create_chunk(addr, size);
	/*chunkmap[addr] = chunk;*/
	if(!init_alloc){
		init_allocs();
		init_alloc = 1;
	}

	if(size >= OBJECT_TRACK_SZ) {
		chunk_addr[offset]=(unsigned long)addr;
		chunk_sz[offset]=size;
		offset++;
		//printf("recording address %u count %u\n", size, alloc_cnt);
		alloc_cnt++;
	}
	return 0;
}


int get_chnk_cnt_frm_map() {

	return chunkmap.size();
}

//Assuming that chunkmap can get data in o(1)
//Memory address range will not work here
//If this method returns NULL, caller needs
//to check if addr is in allocated ranger using
//the o(n) get_chunk_from_map call
void *get_chunk_from_map_o1(void *addr) {

	chunkobj_s *chunk;
	assert(chunkmap.size());
	assert(addr);
	chunk = ( chunkobj_s *)chunkmap[addr];
	return (void *)chunk;
}






