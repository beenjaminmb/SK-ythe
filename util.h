#ifndef _UTIL_
#define _UTIL_

#include <pthread.h>
/*https://stackoverflow.com/questions/2509679/how-to-generate-a-random-number-from-within-a-range */

long range_random(long, struct random_data *, int32_t *);

/* #define _RINGBUFFER 0 */

/* #ifdef _RINGBUFFER */
/* #define memory_barrier()    asm volatile("mfence":::"memory") */
/* #define read_barrier()   asm volatile("lfence":::"memory") */
/* #define write_barrier()   asm volatile("sfence" ::: "memory") */

/* typedef struct ringbuffer_cell { */
/*   void *value; */
/* } ringbuffer_cell_t; */

/* typedef struct ringbuffer { */
/*   ringbuffer_cell_t *head, *tail; */
/*   ringbuffer_cell_t *buf, *end; */
/*   pthread_mutex_t *rb_lock; */
/*   pthread_cond_t *rb_empty; */
/*   pthread_cond_t *rb_full; */
/* } ringbuffer_t; */

/* int ringbuffer_init(ringbuffer_t *rb, int len); */
/* int ringbuffer_destroy(ringbuffer_t *rb); */
/* int ringbuffer_append_single(ringbuffer_t *rb, void *item); */
/* int ringbuffer_remove_single(ringbuffer_t *rb, void **item); */
/* int ringbuffer_empty(ringbuffer_t *rb); */
/* int ringbuffer_full(ringbuffer_t *rb); */
/* #endif */

#endif /* _UTIL_ */
