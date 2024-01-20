#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "my_malloc.h"

// LENGTH refers to the length of the block, including the header
// SIZE refers to the size of the block, not including the header
// length = size + sizeof(block_t)

// block is a block_t * pionting to metadata, ptr is a void * pointing to data

static block_t *head = NULL;

void merge(block_t *block) {
  // Merge block with next block and previous block if possible

  if (block->next &&
      block->next == (block_t *)((char *)block + block->length)) {
    // Merge with next block
    block->length += block->next->length;
    block->next = block->next->next;
    if (block->next) {
      block->next->prev = block;
    }
    printf("Merged with next block\n");
  }

  if (block->prev &&
      block == (block_t *)((char *)block->prev + block->prev->length)) {
    // Merge with previous block
    block->prev->length += block->length;
    block->prev->next = block->next;
    if (block->next) {
      block->next->prev = block->prev;
    }
    printf("Merged with previous block\n");
  }
}

void split(block_t *block, size_t size) {
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
}

void *ff_malloc(size_t size) {
  block_t *block = head;

  while (block) {
    if (block->length >= size + sizeof(block_t)) {
      // We've found the first block that can fit our data

      // If the block is not exactly the right size, we need to split it
      if (block->length > size + sizeof(block_t)) {
        split(block, size);
      }

      return (void *)((char *)block + sizeof(block_t));
    }

    // Try the next block
    block = block->next;
  }

  // If we get here, we didn't find a block that could fit our data
  block = (block_t *)sbrk(size + sizeof(block_t));
  block->length = size + sizeof(block_t);

  return (void *)((char *)block + sizeof(block_t));
}

// Something wrong here
void ff_free(void *ptr) {
  block_t *block = (block_t *)((char *)ptr - sizeof(block_t));

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
        // We've found the right place to insert the block
        block->prev = curr->prev;
        block->next = curr;
        if (curr->prev) {
          curr->prev->next = block;
        }
        curr->prev = block;

        if (curr == head) {
          head = block;
        }

        // Merge the block if possible
        merge(block);
        return;
      }
      last = curr;
      curr = curr->next;
    }
    // We've reached the end of the list, so insert the block at the end
    last->next = block;
    block->prev = last;
    block->next = NULL;

    // Merge the block if possible
    merge(block);
  }
}