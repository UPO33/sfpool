/******************************************************************************
 *           DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *                   Version 2, December 2004
 *
 *  Copyright (C) 2015 Ali Rahbar <junk0xc0de@tuta.io>
 *
 *  Everyone is permitted to copy and distribute verbatim or modified
 *  copies of this license document, and changing it is allowed as long
 *  as the name is changed.
 *
 *           DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *  TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 *
 *  0. You just DO WHAT THE FUCK YOU WANT TO.
 ******************************************************************************/

#pragma once

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef size_t bool_t;

enum SFPOOL_EXPAND_TYPE
{
    SFPOOL_EXPAND_TYPE_ONE = 0,
    SFPOOL_EXPAND_TYPE_TWO = 1,
    SFPOOL_EXPAND_TYPE__UNUSED = (size_t) -1,
};

struct sfpool_page;

struct sfpool
{
    size_t block_size;
    size_t block_count;
    size_t block_distance;

    size_t page_count;
    size_t page_size;

    enum SFPOOL_EXPAND_TYPE expand_type;

    struct sfpool_page* first_page;
    struct sfpool_page* last_page;
    struct sfpool_page* free_pages;
};

struct sfpool_page
{
    struct sfpool* pool;

    struct sfpool_page* prev;
    struct sfpool_page* next;

    struct sfpool_page* prev_free;
    struct sfpool_page* next_free;

    size_t block_count;
    size_t free_count;

    size_t* free_first;

    void* blocks;
};

/* block iterator. is useful for iterating through blocks */
struct sfpool_it
{
    struct sfpool_page* page;
    size_t* header;
    size_t block_pos;
};

/*
 * dis: create and initialize a pool object
 *
 * arg: a pointer to pool object
 * arg: size of each block of pool
 * arg: how many blocks a page must maintain?
 * arg: how should this pool be expanded?
 *
 * ret: 
 */

void sfpool_create (struct sfpool* pool,size_t block_size,size_t page_size,
                              enum SFPOOL_EXPAND_TYPE expand_type);

/*
 * dis: destroy a valid pool object
 *
 * arg: a pointer to pool object
 *
 * ret:
 */
void sfpool_destroy (struct sfpool* pool);

/*
 * dis: allocate a new block from memory pool
 *
 * arg: pointer to pool object
 *
 * ret: returns address of the allocated block if function succeeds,
 *      otherwise returns NULL if it fails for any reason.
 */
void* sfpool_alloc (struct sfpool* pool);

/*
 * dis: free an allocated block
 *
 * arg: pointer to pool object
 * arg: pointer to an allocated block
 *
 * ret: 
 */
void sfpool_free (struct sfpool* pool,void* block);

/*
 * dis: print status of memory pool
 *
 * arg: pointer to pool object
 *
 * ret: 
 */
void sfpool_dump (struct sfpool* pool);

/*
 * dis: initialize an iterator from first block of memory pool
 *
 * arg: pointer to pool object
 * arg: pointer to iterator object
 *
 * ret: a pointer to first used block of memory pool.
 *      it also initialize 'it' iterator object and makes
 *      it point to first block of memory pool.
 */
void* sfpool_it_first (struct sfpool* pool,struct sfpool_it* it);

/*
 * dis: initialize an iterator from last block of memory pool
 *
 * arg: pointer to pool object
 * arg: pointer to iterator object
 *
 * ret: a pointer to first used block of memory pool.
 *      it also initialize 'it' iterator object and makes
 *      it point to last block of memory pool.
 */
void* sfpool_it_last (struct sfpool* pool,struct sfpool_it* it);

/*
 * dis: initialize an iterator from a specified block of memory pool
 *
 * arg: pointer to pool object
 * arg: pointer to iterator object
 * arg: pointer to allocated block
 *
 * ret: a pointer is the same as the given 'block'.
 *      it also initialize 'it' iterator object and makes
 *      it point to the given block of memory pool.
 */
void* sfpool_it_from (struct sfpool* pool,struct sfpool_it* it,void* block);

/*
 * dis: get the next block
 *
 * arg: pointer to iterator object
 *
 * ret: a pointer to the next block. if it exeeds
 *      the last block then it returns NULL.
 *      it also changes 'it' iterator object to point to
 *      the next block.
 */
void* sfpool_it_next (struct sfpool_it* it);

/*
 * dis: get the previous block
 *
 * arg: pointer to iterator object
 *
 * ret: a pointer to the previous block. if it exeeds
 *      the first block then it returns NULL.
 *      it also changes 'it' iterator object to point to
 *      the previous block.
 */
void* sfpool_it_prev (struct sfpool_it* it);

#ifdef __cplusplus
}
#endif /* __cplusplus */

class SFPool
{
	void* Alloc();
	void Free(void* ptr);
}