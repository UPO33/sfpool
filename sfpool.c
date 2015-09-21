#include "sfpool.h"

#define WORD_SIZE ((size_t) - 1)

/*
 * round the given size by system word size (word size is 4 bytes in 32-bits
 * and 8 bytes in 64-bits systems). we'll use this for address alignment.
 */

static size_t round_size (size_t size)
{
    if(size < WORD_SIZE)
    {
        size = WORD_SIZE;
        return size;
    }

    size_t mod = size % WORD_SIZE;

    if(mod != 0)
    {
        size += WORD_SIZE - mod;
    }
    
	return size;
}

struct sfpool* sfpool_create (size_t item_size,size_t item_count)
{
    /* reserve a memory block for the pool object */

    struct sfpool* pool = malloc(sizeof(struct sfpool));

    if(pool == NULL)
    {
        return NULL;
    }

    memset(pool,0,sizeof(struct sfpool));

    /*
     * round the size of each item according system word size and put
     * some extra bytes (paddings) in order to make each item start
     * on word sized boundary address. this will result in higher speed
     * performance. but on the other hand it wastes memory as well.
     */
    
    pool->item_size = round_size(item_size);

    /* check for previous page and find out how big this page should be ? */

    if(pool->pages != NULL) /* so there are already some pages in the pool */
    {
        
    }

    return pool;
}

void sfpool_destroy (struct sfpool* pool)
{
    if(pool == NULL)
    {
        return;
    }

    struct sfpool_page* it,*prev;

    it = pool->pages;

    while(it != NULL)
    {
        prev = it->prev;
        free(it);
        it = prev;
    }
}

static int add_page (struct sfpool* pool,size_t item_count)
{
    size_t raw_size = ((WORD_SIZE + pool->item_size) * item_count) + sizeof(struct sfpool_page);
    struct sfpool_page* page = (struct sfpool_page*) malloc(raw_size);

    if(page == NULL)
    {
        return 1;
    }

    /* initialize the new page */

    page->pool = pool;

    /* link the last page and this page together */

    page->prev = pool->pages;
    page->next = NULL;

    if(pool->pages != NULL)
    {
        pool->pages->next = page;
    }

    /* 
     * if the working page is not set,
     * then set this page to the working page.
     */

    if(pool->working_page == NULL)
    {
        pool->working_page = page;
    }

    pool->pages = page;
    pool->item_count += item_count;
    pool->page_count++;

    /* generate free items */

    unsigned char* start_address = (unsigned char*) &page->items;
    size_t* header = (size_t*) start_address;
    size_t* header_next = NULL;

    /*
     * 'distance' is the distance between this header and next header.
     * the size is not actually in bytes, but it rather was divided
     * by WORD_SIZE to make 'header' address increased by.
     */

    size_t distance = (WORD_SIZE + pool->item_size) / WORD_SIZE;

    for(size_t i = 0;i < (item_count - 1);i++)
    {
        header_next = header + distance;
        *header = (size_t) header_next;
        header = header_next;
    }

    *header = 0x0;

    return 0;
}
