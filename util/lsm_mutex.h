#ifndef FTL_MUTEX_H
#define FTL_MUTEX_H

#include "defs.h"
#include <pthread.h>

#include <pthread.h>

struct ftl_mutex {
  pthread_mutex_t lock;
};

#define FTL_ATOMIC_ADD_FETCH(p, val)  __atomic_fetch_add(p, val, __ATOMIC_SEQ_CST);
#define FTL_ATOMIC_SUB_FETCH(p, val)  __atomic_fetch_sub(p, val, __ATOMIC_SEQ_CST);
#define FTL_ATOMIC_STORE(p, val)   __atomic_store(p, val, __ATOMIC_SEQ_CST);
#define ATOMIC_LOAD(p) __atomic_load_n(p, __ATOMIC_SEQ_CST);

void ftl_mutex_init(struct ftl_mutex *mutex);
void ftl_mutex_lock(struct ftl_mutex *mutex);
void ftl_mutex_unlock(struct ftl_mutex *mutex);
void ftl_mutex_destroy(struct ftl_mutex *mutex);
int64_t ftl_atomic_add_fetch(volatile int*, int64_t);
int64_t ftl_atomic_sub_fetch(volatile int*, int64_t);
#endif
