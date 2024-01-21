#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "my_malloc.h"

// #define DEBUG

// Naming conventions:
// LENGTH refers to the length of the block, including the header
// SIZE refers to the size of the block, not including the header
// length = size + sizeof(block_t)
// block is a block_t * pointing to metadata, ptr is a void * pointing to data

static block_t *head = NULL;
static void *start = NULL; // For storing the first program break

void print_debug(char *prefix) {
#ifdef DEBUG
  printf("[%s] program break: %10p\n", prefix, sbrk(0));
  block_t *block = head;
  printf("[%s] free list: \n", prefix);
  int i = 0;
  while (block) {
    printf("(%d) <%10p> (length: %#lx)\n", i, block, block->length);
    block = block->next;
    i++;
  }
  printf("[%s] data segment size: %lu\n", prefix, get_data_segment_size());
  printf("[%s] data segment free space: %lu\n", prefix,
         get_data_segment_free_space_size());
#endif
}

// Merges (free) block with next block and previous block if possible
void merge(block_t *block) {
  print_debug("before merge");

  if (block->next &&
      block->next == (block_t *)((char *)block + block->length)) {
    // Merge with next block
    block->length += block->next->length;
    block->next = block->next->next;
    if (block->next) {
      block->next->prev = block;
    }
  }

  if (block->prev &&
      block == (block_t *)((char *)block->prev + block->prev->length)) {
    // Merge with previous block
    block->prev->length += block->length;
    block->prev->next = block->next;
    if (block->next) {
      block->next->prev = block->prev;
    }
  }
  print_debug("after merge");
}

// Allocates a block with size size from the free list, splitting if possible
void alloc_from_free(block_t *block, size_t size) {
  print_debug("before alloc from free");

  // Check if we can split the block
  // (NB: need space for header of new block)
  if (block->length <= size + sizeof(block_t) + sizeof(block_t)) {
    if (block->prev) {
      block->prev->next = block->next;
    }
    if (block->next) {
      block->next->prev = block->prev;
    }
    if (block == head) {
      head = block->next;
    }
    return; // Didn't perform a split
  }

  block_t *new_block = (block_t *)((char *)block + size + sizeof(block_t));
  new_block->length = block->length - size - sizeof(block_t);

  new_block->prev = block->prev;
  new_block->next = block->next;

  if (block->prev) {
    block->prev->next = new_block;
  }
  if (block->next) {
    block->next->prev = new_block;
  }
  if (block == head) {
    head = new_block;
  }

  block->length = size + sizeof(block_t);

  print_debug("after alloc from free");
}

// Extends the program break to allocate a new block
void *alloc_new(size_t size) {
  print_debug("before new alloc");

  if (!start) {
    start = sbrk(0); // Record the program break if we haven't already
  }

  block_t *block = (block_t *)sbrk(size + sizeof(block_t));
  if (block == (void *)-1) {
    return NULL; // sbrk failed
  }

  block->length = size + sizeof(block_t);
  block->prev = NULL;
  block->next = NULL;

  print_debug("after new alloc");
  return (void *)((char *)block + sizeof(block_t));
}

// Best-fit malloc algorithm
void *bf_malloc(size_t size) {
  block_t *block = head;
  block_t *best = NULL;
  ssize_t best_length = -1;

  while (block) {
    if (block->length >= size + sizeof(block_t)) {
      if (best_length == -1 || block->length < best_length) {
        best = block;
        best_length = block->length;
      }
    }

    block = block->next;
  }

  if (best) {
    alloc_from_free(best, size);
    return (void *)((char *)best + sizeof(block_t));
  }

  return alloc_new(size);
}

// First-fit malloc algorithm
void *ff_malloc(size_t size) {
  block_t *block = head;

  while (block) {
    if (block->length >= size + sizeof(block_t)) {
      alloc_from_free(block, size);
      return (void *)((char *)block + sizeof(block_t));
    }

    block = block->next;
  }

  return alloc_new(size);
}

// Free a pointer to data (same for ff and bf)
void _free(void *ptr) {
  print_debug("before free");
  block_t *block = (block_t *)((char *)ptr - sizeof(block_t));

  if (!head) {
    head = block;
    block->prev = NULL;
    block->next = NULL;
  } else {
    block_t *curr = head;
    block_t *last = head;

    while (curr) {
      if (curr > block) {
        print_debug("free: before insertion");
        block->prev = curr->prev;
        block->next = curr;
        if (curr->prev) {
          curr->prev->next = block;
        }
        curr->prev = block;

        if (curr == head) {
          head = block;
        }
        print_debug("free: after insertion");

        merge(block);
        return;
      }
      last = curr;
      curr = curr->next;
    }
    print_debug("free: before insertion at end");
    last->next = block;
    block->prev = last;
    block->next = NULL;
    print_debug("free: after insertion at end");

    merge(block);
  }

  print_debug("after free");
}

void ff_free(void *ptr) { _free(ptr); }

void bf_free(void *ptr) { _free(ptr); }

unsigned long get_data_segment_size() {
  if (!start) {
    start = sbrk(0);
  }
  return (unsigned long)sbrk(0) - (unsigned long)start;
}

unsigned long get_data_segment_free_space_size() {
  unsigned long res = 0;
  block_t *block = head;

  while (block) {
    res += block->length;
    block = block->next;
  }

  return res;
}