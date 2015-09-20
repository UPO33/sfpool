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

struct sfpool_page;

struct sfpool
{
    size_t item_size;
    size_t item_count;
    size_t item_align;

    size_t page_count;
    struct sfpool_page* pages;
};

struct sfpool_page
{
    struct sfpool* pool;

    struct sfpool_page* prev;
    struct sfpool_page* next;

    size_t free_count;
    size_t* free_first;

    void* data;

    /* the rest of stuff are arbitrary sized and may not be defined here ! */
};

struct sfpool* sfpool_create (size_t item_size,size_t item_count,size_t item_align);
int sfpool_destroy (struct sfpool* pool);
void* sfpool_alloc (struct sfpool* pool);
void sfpool_free (struct sfpool* pool,void* item);
void sfpool_dump (struct sfpool* pool);

#endif /* SFPOOL_H_ */
