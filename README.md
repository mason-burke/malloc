# 7-malloc

## Maintaining Compaction

When coalescing adjacent free blocks, I would first check to see if both the
next and previous blocks in the heap were free. If they were, instead of
freeing the current block, I can just make the previous block in the heap to
have the size of the previous block's current size plus the block-in-question's
size, and the next block in the heap's size. Of course, this would also render
the next block (that we determined to be free) no longer an autonomous free
block, so it is removed from the free list. In the event that just the next
block is free, we can free the current block and expand it to take its own space
plus the space of the next block, which, again, we would remove from the free
list. If just the previous block is free, then it can be grown to additionally
encompass the size of the block we're currently freeing. In this way, internal
fragmentation is always quite minimal.

## realloc Implementation

The current implementation of realloc is quite simple: if the requested new size
is smaller than the block's current size, then the block can simply be made
smaller, provided that the leftover chunk is big enough to be a free block.
Otherwise, we free the current block, and then use malloc to allocate it. After,
we can copy its payload to the new location.

## Throughput

The throughput for my project has always been 100.00. I've never seen a number
lower than that, so I have no reason to believe that I've done anything
inefficiently (or the throughput-calculating code is incorrect). This code
was compiled with the -O2 flag for increased optimization.

## Unresolved Bugs

In the current implementation, everything works properly. I don't have any
syscalls that are able to be error-checked, and I'm error checking mem_sbrk.

## Optimizations

I did not make any further optimizations.

## Trace Files Performance

My coalescing traces:
coalescing-bal.rep:   99.2%
coalescing2-bal.rep:  99.3%

My realloc traces:
realloc-bal.rep:      50.0%
realloc2-bal.rep:     50.1%
