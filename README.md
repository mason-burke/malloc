# malloc

## overview

This was the penultimate project for my course "Introduction to Computer Systems." The goal of this project was to reimplement the C library functions malloc(), free(), and realloc().

## coalescing and maintaining compaction

When coalescing adjacent free blocks, I would first check to see if both the next and previous blocks in the heap were free. If they were, instead of freeing the current block, I can just make the previous block in the heap to have the size of the previous block plus the size of the block in question, and the next block in the heap's size. Of course, this would also render the next block (that we determined to be free) no longer an autonomous free block, so it is removed from the free list. In the event that just the next block is free, we can free the current block and expand it to take its own space plus the space of the next block, which, again, we would remove from the free list. If just the previous block is free, then it can be grown to additionally encompass the size of the block we're currently freeing. In this way, internal fragmentation is always quite minimal.

## realloc implementation

The current implementation of realloc is quite simple: if the requested new size is smaller than the block's current size, then the block can simply be made smaller, provided that the leftover chunk is big enough to be a free block. Otherwise, we free the current block, and then use malloc to allocate it. After, we can copy its payload to the new location. Initially, I wanted to implement further optimizations, but found them to be rare edge cases that rarely brought benefits. For the sake of keeping the code simple, I opted to not include them. These optimizations mostly focused on recognizing when blocks adjacent to the current blocks were free, allowing us to do larger in-place reallocations.

## Trace Files Performance

Included with the project is a number of trace files to check for the optimization levels of block coalescing and fragmented space after reallocation. Listed below are the most notable trace file results. The thresholds for this assignment were 90% on the coalescing traces, and 45% on the realloc traces.

Coalescing traces:

    coalescing-bal.rep:   99.2%
    coalescing2-bal.rep:  99.3%

Realloc traces:

    realloc-bal.rep:      50.0%
    realloc2-bal.rep:     50.1%
    
## disclaimers

This project had some template code. Of the included files, I wrote the entirety of mm.c, which is where the reimplementations are located. Also, I wrote a few of the inline functions in mminline.h.

## how to use

Download the source files, navigate to the root directory, and run

    make clean all

Now, run the provided file mdriver with the -r flag:

    ./mdriver -r
    
Note that this does support I/O redirection, but I did not find it to be necessary while testing this project. Once in the REPL, there are a number of available commands:

    malloc [id] [size]: allocate a block of memory with an [id] number 
                        (must be unique to the blocks currently allocated) with a certain [size]-byte payload.
    free [id]: free a block with a given [id].
    realloc [id] [size]: reallocate a block with a given [id] to a new [size].
    print: prints out the state of the heap.
    EOF: breaks out of the REPL, cleans up heap.
