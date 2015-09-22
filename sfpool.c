#include "sfpool.h"

#define WORD_SIZE (sizeof(size_t))

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

struct sfpool* sfpool_create (size_t block_size,size_t page_size,enum SFPOOL_EXPAND_FACTOR expand_factor)
{
    /* reserve a memory block for the pool object */
    struct sfpool* pool = malloc(sizeof(struct sfpool));

    if(pool == NULL)
    {
        return NULL;
    }

    memset(pool,0,sizeof(struct sfpool));

    /*
     * round the size of each block according to system word size and put
     * some extra bytes (paddings) in order to make each block start
     * on word sized boundary address. this will result in higher speed
     * performance. but on the other hand it wastes memory as well.
     */
    pool->block_size = round_size(block_size);
    pool->page_size = page_size;
    pool->expand_factor = expand_factor;

    return pool;
}

void sfpool_destroy (struct sfpool* pool)
{
    if(pool == NULL)
    {
        return;
    }

    struct sfpool_page* it,*prev;

    it = pool->all_pages;

    while(it != NULL)
    {
        prev = it->prev;
        free(it);
        it = prev;
    }
}

static struct sfpool_page* add_page (struct sfpool* pool,enum SFPOOL_EXPAND_FACTOR expand_factor)
{
    size_t raw_size = ((WORD_SIZE + pool->block_size) * pool->page_size) +
                      sizeof(struct sfpool_page);

    struct sfpool_page* page = (struct sfpool_page*) malloc(raw_size);

    if(page == NULL)
    {
        return NULL;
    }

    /* initialize the new page */
    page->pool = pool;

    /* add this page at the start of all_pages list */
    page->next = pool->all_pages;
    page->prev = NULL;

    if(pool->all_pages != NULL)
    {
        pool->all_pages->prev = page;
    }

    /* add this page at the end of free_pages list */
    page->next_free = pool->free_pages;
    page->prev_free = NULL;

    if(pool->free_pages != NULL)
    {
        pool->free_pages->prev_free = page;
    }

    page->block_count = pool->page_size;
    page->free_count = pool->page_size;

    pool->all_pages = page;
    pool->free_pages = page;

    pool->block_count += pool->page_size;
    pool->page_count++;

    /* generate free blocks */
    unsigned char* start_address = (unsigned char*) &page->blocks;
    size_t* header = (size_t*) start_address;
    size_t* header_next = NULL;

    /*
     * 'distance' is the distance between this header and next header.
     * the size is not actually in bytes, but it rather was divided
     * by WORD_SIZE to make 'header' address increased by.
     */
    size_t distance = (WORD_SIZE + pool->block_size) / WORD_SIZE;

    for(size_t i = 0;i < (pool->page_size - 1);i++)
    {
        header_next = header + distance;
        *header = (size_t) header_next;
        header = header_next;
    }

    *header = 0x0;
    page->free_first = (size_t*) start_address;

    return page;
}

static void delete_page (struct sfpool* pool,struct sfpool_page* page)
{
    if(page->prev) page->prev->next = page->next;
    if(page->next) page->next->prev = page->prev;

    if(page->prev_free) page->prev_free->next_free = page->next_free;
    if(page->next_free) page->next_free->prev_free = page->prev_free;

    /* if this page is the first page */
    if(pool->all_pages == page)
    {
        pool->all_pages = page->next;
    }
    /* if this page is the first free page */
    if(pool->free_pages == page)
    {
        pool->free_pages = page->next;
    }

    pool->block_count -= page->block_count;
    pool->page_count--;

    /* if this was the last page */
    if(pool->page_count == 0)
    {
        pool->page_size = page->block_count;
        return;
    }
}

void* sfpool_alloc (struct sfpool* pool)
{
    /* get the current working page */
    struct sfpool_page* page = pool->free_pages;

    /* check if we already have a free page */
    if(page != NULL)
    {
        /*
         * check if the page has any free blocks to be allocated,
         * if so, then allocate it !
         */

        if(page->free_count != 0)
        {

            /* get the address of the first free block */
            size_t* block = (size_t*) page->free_first;

            /*
             * mark the first free block as used ,
             * then put the next free block as the new first free block.
             */
            page->free_count--;
            page->free_first = (size_t*) *block;

            /* 
             * put the address of the page in the header of the block.
             * this will be useful when we want to free an block.
             */
            *block = (size_t) page;
    
            /* the block lives just a word size after the header :) */
            return (void*) (block + 1);
         }
    }

    /*
     *  it seems that we don't have any free pages!
     *  this only happens when:
     *
     *  - first time of calling sfpool_alloc()
     *  - we have already freed all the pages.
     *  - all pages are full.
     *  - add_page() has failed.
     */

    /* if the reason that we're here is page == NULL */
    if(page == NULL)
    {
        /* request a new page */
        page = add_page(pool,pool->expand_factor);

        return sfpool_alloc(pool);
    }

    /* check if there is a next free page */
    if(page->next_free != NULL)
    {
       /*
        * put this page out of our free page list
        * and replace it with the next free page
        */

        page->next_free->prev_free = NULL;
        pool->free_pages = page->next_free;

        page->next_free = NULL;
        page->prev_free = NULL;

        return sfpool_alloc(pool);
    }
    /* if there is not any pages then create a new one */
    else
    {
        /* request a new page */
        page = add_page(pool,pool->expand_factor);

        /* if the requested page could not be created for any reason */
        if(page == NULL)
        {
            return NULL;
        }

        return sfpool_alloc(pool);
    }
}

void sfpool_free (struct sfpool* pool,void* ptr)
{
    /* header lives just a word size before the block */
    size_t* header = ((size_t*) (ptr)) - 1;

    /* header's data is ae address to the owner page */
    struct sfpool_page* page = (struct sfpool_page*) *header;

    /*
     * make this block free by inserting the address of other free block in it.
     * then put this block as our new free block.
     */
    *header = (size_t) page->free_first;
    page->free_first = header;
    page->free_count++;

    /* if the owner page is entirely free */
    if(page->free_count == page->block_count)
    {
        delete_page(pool,page);
    }
}

void sfpool_dump (struct sfpool* pool)
{
    printf(
    "== SFPOOL ==\n"
    "block_size      : %u\n"
    "block_count     : %u\n"
    "page_count     : %u\n"
    "expand_factor  : %u\n"
    "============\n",
    pool->block_size,
    pool->block_count,
    pool->page_count,
    pool->expand_factor);

    struct sfpool_page* page = pool->all_pages;
    size_t* header = 0;

    for(size_t i = 0;i < pool->page_count;i++)
    {
        printf("PAGE { ");
        header = &page->blocks;

        for(size_t count = 0;count < page->block_count;count++)
        {
            if(*header == (size_t) page)
            {
                printf("1");
            }
            else
            {
                printf("0");
            }

            header = header + (WORD_SIZE + pool->block_size) / WORD_SIZE;
        }

        page = page->next;

        printf(" }\n");
    }
}

static void* first_used_block (struct sfpool* pool,struct sfpool_page* page,void* block)
{
    size_t* header = ((size_t*) block) - 1;
    size_t count = 0;
    size_t distance = (WORD_SIZE + pool->block_size) / WORD_SIZE;

    /* start from the given page and check every block's header */
    while(page != NULL)
    {
        count = 0;
        header = ((size_t*) block) - 1;
        
        /* check all blocks of the page */
        while(count != page->block_count)
        {
            /* is this a used block */
            if(*header == ((size_t) page))
            {
                /* yes! this is a used block! */
                return (void*) (header + 1);
            }

            header = header + distance;
            count++;
        }

        page = page->next;
    }

    /* unfortunately we have no used blocks */
    return NULL;
}

void* sfpool_it_init (struct sfpool* pool,struct sfpool_it* it,void* block)
{
    struct sfpool_page* page;
    size_t* header;

    memset(it,0,sizeof(struct sfpool_it));

    /* if the block is given as NULL */
    if(block == NULL)
    { 
        /* use the first used block as default */
        page = pool->all_pages;
        block = ((size_t*) (&page->blocks)) + 1;
    }

    /* if the block is an allocated block */
    header = ((size_t*) block) - 1;
    page = (struct sfpool_page*) *header;
    block = first_used_block(pool,page,block);

    /* unfortunately we have no used blocks */
    if(block == NULL)
    {
        return NULL;
    }

    /* now initialize the iterator object */
    it->page = page;
    it->header = header;
    it->block_size = pool->block_size;

    return block;
}

void* sfpool_it_next (struct sfpool_it* it)
{    
    void* block = ((size_t*) (it->header)) + 1;
    block = first_used_block(it->page->pool,it->page,block);

    /* unfortunately we have no used blocks */
    if(block == NULL)
    {
        memset(it,0,sizeof(struct sfpool_it));
        return NULL;
    }

    /* now initialize the iterator object */

    it->header = ((size_t*) block) - 1;
    it->page = (struct sfpool_page*) *it->header;

   return block;
}
