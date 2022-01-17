#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * BEFORE GETTING STARTED:
 *
 * Familiarize yourself with the functions and constants/variables
 * in the following included files.
 * This will make the project a LOT easier as you go!!
 *
 * The diagram in Section 4.1 (Specification) of the handout will help you
 * understand the constants in mm.h
 * Section 4.2 (Support Routines) of the handout has information about
 * the functions in mminline.h and memlib.h
 */
#include "./memlib.h"
#include "./mm.h"
#include "./mminline.h"

block_t *prologue;
block_t *epilogue;

// rounds up to the nearest multiple of WORD_SIZE
static inline size_t align(size_t size) {
    return (((size) + (WORD_SIZE - 1)) & ~(WORD_SIZE - 1));
}

/*
 * creates a new block
 * arguments: aligned_size: the desired aligned size for the block
 *            last_free: an int reflecting if the last block before the
 *                        epilogue is allocated (0 if free, 1 if allocated)
 * returns: a pointer to the newly-allocated block's payload,
 *           or NULL if an error occurred
 */
void *build_new_block(size_t aligned_size, int last_allocated) {
    // new block will begin where epilogue was
    block_t *new_block;
    long err;

    // if the last block is free
    if (!last_allocated) {
        // then start the new block there, and expand the heap to complete it
        new_block = block_prev(epilogue);
        assert(aligned_size > block_prev_size(epilogue));
        err =
            (long)mem_sbrk((int)aligned_size - (int)block_prev_size(epilogue));
    }
    // if the last block is allocated
    else {
        // expand heap as normal
        new_block = epilogue;
        err = (long)mem_sbrk((int)aligned_size);
    }
    // error checking mem_sbrk
    if (err == -1) {
        return NULL;
    }

    // setup block
    block_set_size_and_allocated(new_block, aligned_size, 1);

    // move epilogue
    epilogue = block_next(new_block);
    block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);

    // return pointer to block's payload
    return new_block->payload;
}

/*
 * given a free block, this function allocates in that block
 *  also, if the block is big enough, then it is
 *   split into allocated and free portions
 * arguments: pulled_block: a block that has been removed from the free list
 *            new_size: the size to allocate
 *            old_size: the size of the existing block
 * returns: a pointer to the allocated block's payload
 */
void *allocate_and_split(block_t *b, size_t new_size, size_t old_size) {
    // if we can split the block
    if (old_size >= new_size + MINBLOCKSIZE) {
        // split up the block into free and allocated portions
        block_set_size_and_allocated(b, new_size, 1);
        block_set_size_and_allocated(block_next(b), old_size - new_size, 0);

        // insert the free part into the free list
        insert_free_block(block_next(b));

        return b->payload;
    }

    // otherwise just set the allocated bit
    block_set_size_and_allocated(b, old_size, 1);
    return b->payload;
}

/*
 *                             _       _ _
 *     _ __ ___  _ __ ___     (_)_ __ (_) |_
 *    | '_ ` _ \| '_ ` _ \    | | '_ \| | __|
 *    | | | | | | | | | | |   | | | | | | |_
 *    |_| |_| |_|_| |_| |_|___|_|_| |_|_|\__|
 *                       |_____|
 *
 * initializes the dynamic storage allocator (allocate initial heap space)
 * arguments: none
 * returns: 0, if successful
 *         -1, if an error occurs
 */
int mm_init(void) {
    // setup prologue
    if ((long)(prologue = (block_t *)mem_sbrk(TAGS_SIZE)) < 0) {
        return -1;
    }
    block_set_size_and_allocated(prologue, TAGS_SIZE, 1);

    // setup epilogue
    if ((long)(epilogue = (block_t *)mem_sbrk(TAGS_SIZE)) < 0) {
        return -1;
    }
    block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);

    // setup flist_first
    flist_first = NULL;

    return 0;
}

/*     _ __ ___  _ __ ___      _ __ ___   __ _| | | ___   ___
 *    | '_ ` _ \| '_ ` _ \    | '_ ` _ \ / _` | | |/ _ \ / __|
 *    | | | | | | | | | | |   | | | | | | (_| | | | (_) | (__
 *    |_| |_| |_|_| |_| |_|___|_| |_| |_|\__,_|_|_|\___/ \___|
 *                       |_____|
 *
 * allocates a block of memory and returns a pointer to that block's payload
 * arguments: size: the desired payload size for the block
 * returns: a pointer to the newly-allocated block's payload (whose size
 *          is a multiple of ALIGNMENT), or NULL if an error occurred
 */
void *mm_malloc(size_t size) {
    // check for nonzero & nonnegative size
    if (size < 1) {
        return NULL;
    }

    // align the size
    size_t aligned_size = align(size) + TAGS_SIZE;

    // if no free list, build a new block
    if (flist_first == NULL) {
        return build_new_block(aligned_size, 1);
    }

    // the full size of the existing block (to check if we need to split)
    size_t existing_size;
    block_t *curr = flist_first;

    // if the head of the free list is big enough
    if ((existing_size = block_size(flist_first)) >= aligned_size) {
        // then pull it from the free list
        pull_free_block(flist_first);

        // allocate and potentially split the block
        return allocate_and_split(curr, aligned_size, existing_size);
    }

    /* otherwise, iterate through free list until we either:
     *  1. find a block that's big enough (then allocate and break)
     *  2. reach the head of the list
     *      (new block, no sufficient free block found)
    */
    curr = block_flink(curr);

    while (curr != flist_first) {
        // if the current block is free and big enough
        if ((existing_size = block_size(curr)) >= aligned_size) {
            // then pull it from the free list
            pull_free_block(curr);

            // allocate and potentially split the block
            return allocate_and_split(curr, aligned_size, existing_size);
        }

        // otherwise, grab the next block in the free list
        curr = block_flink(curr);
    }

    // check if the last block in heap is free
    if (!block_prev_allocated(epilogue)) {
        // then pull it from the free list
        pull_free_block(block_prev(epilogue));

        /* let build_new_block know that the allocation can start at
         * the last block, minimizing the amount by which we grow the heap */
        return build_new_block(aligned_size, 0);
    }

    // if a sufficient block wasn't found, build a new one
    return build_new_block(aligned_size, 1);
}

/*                              __
 *     _ __ ___  _ __ ___      / _|_ __ ___  ___
 *    | '_ ` _ \| '_ ` _ \    | |_| '__/ _ \/ _ \
 *    | | | | | | | | | | |   |  _| | |  __/  __/
 *    |_| |_| |_|_| |_| |_|___|_| |_|  \___|\___|
 *                       |_____|
 *
 * frees a block of memory, enabling it to be reused later
 * arguments: ptr: pointer to the block's payload
 * returns: nothing
 */
void mm_free(void *ptr) {
    // get block from payload
    block_t *b = payload_to_block(ptr);

    // determine if the next and previous blocks are free
    size_t next_free = !block_next_allocated(b);
    size_t prev_free = !block_prev_allocated(b);

    // if the previous and next blocks are free
    if (next_free && prev_free) {
        // pull the next block from the free list
        pull_free_block(block_next(b));

        // update the previous block's size to be the size of all 3
        block_set_size_and_allocated(
            block_prev(b),
            block_prev_size(b) + block_size(b) + block_next_size(b), 0);
    }
    // if just the next block is free
    else if (next_free) {
        // pull the next block from the free list
        pull_free_block(block_next(b));

        // make the current block the size of itself plus the next block
        block_set_size_and_allocated(b, block_size(b) + block_next_size(b), 0);

        // add to the free list
        insert_free_block(b);
    }
    // if just the previous block is free
    else if (prev_free) {
        // update its size with its own size and the size of the current block
        block_set_size_and_allocated(block_prev(b),
                                     block_size(b) + block_prev_size(b), 0);
    }
    // if neither neighboring block is free
    else {
        // clear block
        block_set_allocated(b, 0);

        // add to the free list
        insert_free_block(b);
    }
}

/*
 *                                            _ _
 *     _ __ ___  _ __ ___      _ __ ___  __ _| | | ___   ___
 *    | '_ ` _ \| '_ ` _ \    | '__/ _ \/ _` | | |/ _ \ / __|
 *    | | | | | | | | | | |   | | |  __/ (_| | | | (_) | (__
 *    |_| |_| |_|_| |_| |_|___|_|  \___|\__,_|_|_|\___/ \___|
 *                       |_____|
 *
 * reallocates a memory block to update it with a new given size
 * arguments: ptr: a pointer to the memory block's payload
 *            size: the desired new payload size
 * returns: a pointer to the new memory block's payload
 */
void *mm_realloc(void *ptr, size_t size) {
    // if no requested size, free the block
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    // align size
    size_t new_size = align(size) + TAGS_SIZE;

    // if null pointer, make new block
    if (ptr == NULL) {
        return mm_malloc(new_size);
    }

    // get block
    block_t *b = payload_to_block(ptr);
    size_t b_size = block_size(b);

    // if nothing is to be done
    if (new_size == b_size) {
        return ptr;
    }

    // ------------------- allocing into smaller size ----------------------
    if (new_size < b_size) {
        // see if we can split the block
        if (b_size - new_size >= MINBLOCKSIZE) {
            // update block with new size
            block_set_size_and_allocated(b, new_size, 1);

            // create free block
            block_set_size_and_allocated(block_next(b), b_size - new_size, 0);
            insert_free_block(block_next(b));
        }

        // if we can't split the block, then we don't need to change anything!
        return ptr;
    }

    // ------------------- otherwise, grow the heap -----------------------
    // store the payload
    int n = new_size - TAGS_SIZE;
    char payload_holder[n];
    memcpy(payload_holder, ptr, n);

    // free the block
    mm_free(ptr);

    // find another free block to allocate in, or make a new block
    ptr = mm_malloc(new_size);

    // bring over payload
    memcpy(ptr, payload_holder, n);
    return ptr;
}
