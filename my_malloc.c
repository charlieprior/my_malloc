#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "my_malloc.h"

// #define DEBUG

// LENGTH refers to the length of the block, including the header
// SIZE refers to the size of the block, not including the header
// length = size + sizeof(block_t)

// block is a block_t * pionting to metadata, ptr is a void * pointing to data

static block_t *head = NULL;
static void *start = NULL;

void print_debug(char *prefix) {
#ifdef DEBUG
  printf("%s\n", prefix);
#endif
}

void merge(block_t *block) {
  print_debug("before merge");
  // Merge block with next block and previous block if possible

  if (block->next &&
      block->next == (block_t *)((char *)block + block->length)) {
    // Merge with next block
    block->length += block->next->length;
    block->next = block->next->next;
    if (block->next) {
      block->next->prev = block;
    }
    // printf("Merged with next block\n");
  }

  if (block->prev &&
      block == (block_t *)((char *)block->prev + block->prev->length)) {
    // Merge with previous block
    block->prev->length += block->length;
    block->prev->next = block->next;
    if (block->next) {
      block->next->prev = block->prev;
    }
    // printf("Merged with previous block\n");
  }
  print_debug("after merge");
}

void split(block_t *block, size_t size) {
  // printf("splitting block %p with new length %#lx\n", block,
  //        size + sizeof(block_t));
  print_debug("before split");

  // Check if we can split the block
  // (need space for header of new block)
  if (block->length <= size + sizeof(block_t) + sizeof(block_t)) {
    // printf("Cannot split block\n");

    // Update pointers
    if (block->prev) {
      block->prev->next = block->next;
    }
    if (block->next) {
      block->next->prev = block->prev;
    }
    if (block == head) {
      head = block->next;
    }
    return;
  }

  block_t *new_block = (block_t *)((char *)block + size + sizeof(block_t));
  new_block->length = block->length - size - sizeof(block_t);

  // Update the linked list pointers
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

  // printf(
  //     "Splitting block %p into blocks %p (length %#lx) and %p (length
  //     %#lx)\n", block, block, block->length, new_block, new_block->length);

  print_debug("after split");
}

void *ff_malloc(size_t size) {
  print_debug("before malloc");
  block_t *block = head;

  while (block) {
    if (block->length >= size + sizeof(block_t)) {
      // We've found the first block that can fit our data

      // If the block is not exactly the right size, we need to split it
      if (block->length > size + sizeof(block_t)) {
        split(block, size);
      } else {
        // Update the linked list pointers
        if (block->prev) {
          block->prev->next = block->next;
        }
        if (block->next) {
          block->next->prev = block->prev;
        }
        if (block == head) {
          head = block->next;
        }
      }
      // printf("Allocating block %p with length %#lx bytes\n", block,
      //        block->length);

      print_debug("after malloc");
      return (void *)((char *)block + sizeof(block_t));
    }

    // Try the next block
    block = block->next;
  }

  // If we get here, we didn't find a block that could fit our data
  // First record the program break
  if (!start) {
    start = sbrk(0);
  }

  block = (block_t *)sbrk(size + sizeof(block_t));
  if (block == (void *)-1) {
    // printf("sbrk failed\n");
    return NULL;
  }
  block->length = size + sizeof(block_t);
  block->prev = NULL;
  block->next = NULL;

  // printf("Allocating block %p with length %#lx bytes\n", block,
  // block->length);
  print_debug("after malloc");
  return (void *)((char *)block + sizeof(block_t));
}

void ff_free(void *ptr) {
  print_debug("before free");
  block_t *block = (block_t *)((char *)ptr - sizeof(block_t));
  // printf("Freeing block %p with length %#lx bytes\n", block, block->length);

  // Add the block to the linked list
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
        // We've found the right place to insert the block
        block->prev = curr->prev;
        block->next = curr;
        if (curr->prev) {
          curr->prev->next = block;
        }
        curr->prev = block;

        if (curr == head) {
          // printf("head changed\n");
          head = block;
        }
        print_debug("free: after insertion");

        // Merge the block if possible
        merge(block);
        return;
      }
      last = curr;
      curr = curr->next;
    }
    print_debug("free: before insertion at end");
    // We've reached the end of the list, so insert the block at the end
    last->next = block;
    block->prev = last;
    block->next = NULL;
    print_debug("free: after insertion at end");

    // Merge the block if possible
    merge(block);
  }

  print_debug("after free");
}

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