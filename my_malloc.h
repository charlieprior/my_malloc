#ifndef __MY_MALLOC_H__
#define __MY_MALLOC_H__

#include <stdlib.h>

typedef struct block {
  size_t length; // The length of the block, including the header
  struct block *prev;
  struct block *next;
} block_t;

// First Fit malloc/free
void *ff_malloc(size_t size);
void alloc_from_free(block_t *block, size_t size);
void ff_free(void *ptr);

// Best Fit malloc/free
void *bf_malloc(size_t size);
void bf_free(void *ptr);

// Performance Functions
unsigned long get_data_segment_size();
unsigned long get_data_segment_free_space_size();

#endif
