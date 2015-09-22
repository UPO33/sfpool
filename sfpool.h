/*
 * Simple Fast Memory Pool
 */

#ifndef SFPOOL_H
#define SFPOOL_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum SFPOOL_EXPAND_FACTOR
{
    SFPOOL_EXPAND_FACTOR_ONE = 0,
    SFPOOL_EXPAND_FACTOR_TWO = 1,
    SFPOOL_EXPAND_FACTOR__UNUSED = (size_t) -1,
};

struct sfpool_page;

struct sfpool
{
    size_t block_size;
    size_t block_count;

    size_t page_count;
    size_t page_size;

    enum SFPOOL_EXPAND_FACTOR expand_factor;

    struct sfpool_page* all_pages;
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

    /* the rest of stuff are arbitrary sized and may not be defined here ! */
};

struct sfpool* sfpool_create (size_t block_size,size_t page_size,enum SFPOOL_EXPAND_FACTOR expand_factor);
void sfpool_destroy (struct sfpool* pool);
void* sfpool_alloc (struct sfpool* pool);
void sfpool_free (struct sfpool* pool,void* block);
void sfpool_dump (struct sfpool* pool);
void* sfpool_iterate (struct sfpool* pool,void* block);

#endif /* SFPOOL_H_ */
