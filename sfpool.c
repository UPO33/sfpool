#include "sfpool.h"

/*
 * round the given size by system word size (word size is 4 bytes in 32-bits
 * and 8 bytes in 64-bits systems). we'll use this for address alignment.
 */
static size_t round_size (size_t size)
{
    if(size < sizeof(size_t))
    {
        size = sizeof(size_t);
        return size;
    }

    size_t mod = size % sizeof(size_t);

    if(mod != 0)
    {
        size += sizeof(size_t) - mod;
    }

    return size;
}

void sfpool_create (struct sfpool* pool,size_t block_size,size_t page_size,enum SFPOOL_EXPAND_TYPE expand_type)
{
    memset(pool,0,sizeof(struct sfpool));

    /*
     * round the size of each block according to system word size and put
     * some extra bytes (paddings) in order to make each block start
     * on word sized boundary address. this will result in higher speed
     * performance. but on the other hand it wastes memory as well.
     */
    printf("block_size  %d\n",block_size);
    printf("rblock_size %d\n",round_size(block_size));
    pool->block_size = round_size(block_size);
    pool->page_size = page_size;
    pool->expand_type = expand_type;

    /*
     * 'distance' is the distance between this header and next header.
     * the size is not actually in bytes, but it rather was divided
     * by sizeof(size_t) to make 'header' address increased by.
     */
    pool->block_distance = (sizeof(size_t) + pool->block_size) / sizeof(size_t);
}

void sfpool_destroy (struct sfpool* pool)
{
    /* check if the memory pool is valid? */
    if(pool == NULL)
    {
        return;
    }

    /* check if the memory pool is valid? */
    struct sfpool_page* it,*next;

    /* get the first page */
    it = pool->first_page;

    /* free all pages */
    while(it != NULL)
    {
        next = it->next;
        free(it);
        it = next;
    }
}

static struct sfpool_page* add_page (struct sfpool* pool,
                                     enum SFPOOL_EXPAND_TYPE expand_type)
{
    size_t raw_size = ((sizeof(size_t) + pool->block_size) * pool->page_size) +
                      sizeof(struct sfpool_page);

    struct sfpool_page* page = (struct sfpool_page*) malloc(raw_size);

    if(page == NULL)
    {
        return NULL;
    }

    /* initialize the new page */
    page->pool = pool;

    /* add this page after the last page */
    page->next = NULL;
    page->prev = pool->last_page;

    /* check if we have last page */
    if(pool->last_page != NULL)
    {
        pool->last_page->next = page;
    }
    else
    {
        /* 
         * we don't have last page.
         * it means we don't have a first page.
         * make this page as first page as well.
         */

        pool->first_page = page;
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

    pool->last_page = page;
    pool->free_pages = page;

    pool->block_count += pool->page_size;
    pool->page_count++;

    /* generate the free blocks */
    size_t* header = (size_t*) &page->blocks;
    size_t* header_next = NULL;

    for(size_t i = 0;i < (pool->page_size - 1);i++)
    {
        /* address of the next block header */
        header_next = header + pool->block_distance;

        /* put the address of the next block header in the block header */
        *header = (size_t) header_next;
        
        /*
         * put the address of the page in the block's free space.
         * because no one still uses the block's free space
         * hence we store the address of the block's owner page
         * in there. this address will be used by
         * sfpool_it_next() and sfpool_it_prev().
         */
        *(header + 1) = (size_t) page;

        /* goto next header */
        header = header_next;
    }

    /* the last free header must point to NULL */
    *header = 0x0;
    page->free_first = (size_t*) &page->blocks;

    return page;
}

static void delete_page (struct sfpool* pool,struct sfpool_page* page)
{
    if(page->prev) page->prev->next = page->next;
    if(page->next) page->next->prev = page->prev;

    if(page->prev_free) page->prev_free->next_free = page->next_free;
    if(page->next_free) page->next_free->prev_free = page->prev_free;

    /* if this page is the first page */
    if(pool->first_page == page)
    {
        pool->first_page = page->next;
    }
    /* if this page is the last page */
    if(pool->last_page == page)
    {
        pool->last_page = page->prev;
    }
    /* if this page is the first free page */
    if(pool->free_pages == page)
    {
        pool->free_pages = page->next;
    }

    pool->block_count -= page->block_count;
    pool->page_count--;
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
        page = add_page(pool,pool->expand_type);

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
        page = add_page(pool,pool->expand_type);

        /* if the requested page could not be created for any reason */
        if(page == NULL)
        {
            return NULL;
        }

        return sfpool_alloc(pool);
    }
}

void sfpool_free (struct sfpool* pool,void* block)
{
    /* header lives just a word size before the block */
    size_t* header = ((size_t*) (block)) - 1;

    /* header's data is an address to the owner page */
    struct sfpool_page* page = (struct sfpool_page*) *header;

    /*
     * make this block free by inserting the address
     * of other free block in it. then put this block
     * as our new free block.
     */
    *header = (size_t) page->free_first;

    /*
     * put the address of the page in the block's free space.
     * because no one still uses the block's free space
     * hence we store the address of the block's owner page
     * in there. this address will be used by
     * sfpool_it_next() and sfpool_it_prev().
     */
    *(header + 1) = (size_t) page;

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
    /* print status of memory pool */
    printf(
    "== SFPOOL ==\n"
    "block_size     : %u\n"
    "block_count    : %u\n"
    "page_count     : %u\n"
    "expand_type  : %u\n"
    "============\n",
    pool->block_size,
    pool->block_count,
    pool->page_count,
    pool->expand_type);

    struct sfpool_page* page = pool->first_page;
    size_t* header = 0;

    /* iterator through all pages ... */
    for(size_t i = 0;i < pool->page_count;i++)
    {
        printf("PAGE { %p : ",page);

        /* get first block header of the page */
        header = (size_t*) &page->blocks;

        /* walk through all blocks and print whether if they're used or not */
        for(size_t count = 0;count < page->block_count;count++)
        {
            /* this block is used */
            if(*header == (size_t) page)
            {
                printf("1");
            }
            /* this block is free */
            else
            {
                printf("0");
            }

            /* goto to next block */
            header = header + (sizeof(size_t) + pool->block_size) / sizeof(size_t);
        }

        printf(" }\n");

        /* goto to next page */
        page = page->next;
    }
}

static size_t* next_used (struct sfpool_page* page,size_t* header,
                         size_t *the_pos)
{
    size_t pos = *the_pos;
    size_t max = page->block_count;
    size_t distance = page->pool->block_distance;

    while(1)
    {
        /* check if we're still in the page boundary */
        while(pos < max)
        {
            /* if the header is a used kind */
            if(*header == (size_t) page)
            {
                /* we found a used block! now initialize the iterator object */
                *the_pos = pos;

                return header;
            }

            /* go to the next header */
            pos++;
            header += distance;
        }

        /* turn the page :) */
        page = page->next;

        /* now is this page valid? */
        if(page == NULL)
        {
            break;
        }

        /* now start from first header block of the page */
        header = (size_t*) &page->blocks;
        pos = 0;
        max = page->block_count;
    }

    return NULL;
}

static size_t* prev_used (struct sfpool_page* page,size_t* header,size_t *the_pos)
{
    size_t pos = *the_pos;
    size_t min = (size_t) (0 - 1);
    size_t distance = page->pool->block_distance;

    while(1)
    {
        /* check if we're still in the page boundary */
        while(pos != min)
        {
            /* if the header is a used kind */
            if(*header == (size_t) page)
            {
                /* we found a used block! now initialize the iterator object */
                *the_pos = pos;

                return header;
            }

            /* go to the previous header */
            pos--;
            header -= distance;
        }

        /* turn the page backward */
        page = page->prev;

        /* now is this page valid? */
        if(page == NULL)
        {
            break;
        }

        /* now start from last header block of the page */
        pos = page->block_count - 1;
        header = (size_t*) ((&page->blocks) + (distance * pos));
    }

    return NULL;
}

void* sfpool_it_first (struct sfpool* pool,struct sfpool_it* it)
{
    /* get first page of memory pool */
    struct sfpool_page* page = pool->first_page;

    /* check if the page is valid? */
    if(page == NULL)
    {
        return NULL;
    }

    /* get first block header of the page */
    size_t* header = (size_t*) &page->blocks;
    size_t pos = 0;

    /* find first used block header after the current block header */
    header = next_used(page,header,&pos);

    /*
     * if next_used() not returned NULL,
     * it means it has found a used block header
     */
    if(header != NULL)
    {
        /* save the current status into the iterator object */
        it->page = (struct sfpool_page*) *header;
        it->header = header;
        it->block_pos = pos;

        return (header + 1);
    }

    /* fill the iterator with zero */
    memset(it,0,sizeof(struct sfpool_it));

    return NULL;
}

void* sfpool_it_last (struct sfpool* pool,struct sfpool_it* it)
{
    /* get last page of memory pool */
    struct sfpool_page* page = pool->last_page;

    /* check if the page is valid? */
    if(page == NULL)
    {
        return NULL;
    }

    /* get last block header of the page */
    size_t* header = (size_t*) (&page->blocks) + (pool->block_distance * (page->block_count - 1));
    size_t pos = page->block_count - 1;

    /* find first used block header after the current block header */
    header = prev_used(page,header,&pos);

    /*
     * if prev_used() not returned NULL,
     * it means it has found a used block header
     */

    if(header != NULL)
    {
        /* save the current status into the iterator object */
        it->page = (struct sfpool_page*) *header;
        it->header = header;
        it->block_pos = pos;

        return (header + 1);
    }

    /* fill the iterator with zero */
    memset(it,0,sizeof(struct sfpool_it));

    return NULL;
}

void* sfpool_it_block (struct sfpool* pool,struct sfpool_it* it,void* block)
{
    struct sfpool_page* page;
    size_t* header = ((size_t*) block ) - 1;
    size_t pos;

    /* get the owner page of the header */
    page = (struct sfpool_page*) *header;
    
    /* get position of the header in the page */
    pos = (((size_t) header) - ((size_t) &page->blocks)) / (sizeof(size_t) + pool->block_size);

    /* save the current status into the iterator object */
    it->page = page;
    it->header = header;
    it->block_pos = pos;

    return (header + 1);
}

void* sfpool_it_next (struct sfpool_it* it)
{
    /* load the last status from the iterator object */
    struct sfpool_page* page = it->page;
    size_t* header = it->header + page->pool->block_distance;
    size_t pos = it->block_pos + 1;

    /* find first used block header after the current block header */
    header = next_used(page,header,&pos);

    /*
     * if next_used() not returned NULL,
     * it means it has found a used block header
     */

    if(header != NULL)
    {
        /* save the current status into the iterator object */
        it->page = (struct sfpool_page*) *header;
        it->header = header;
        it->block_pos = pos;

        return (header + 1);
    }

    /* fill the iterator with zero */
    memset(it,0,sizeof(struct sfpool_it));

    return NULL;
}

void* sfpool_it_prev (struct sfpool_it* it)
{
    /* load the last status from the iterator object */
    struct sfpool_page* page = it->page;
    size_t* header = it->header - page->pool->block_distance;
    size_t pos = it->block_pos - 1;

    /* find first used block header after the current block header */
    header = prev_used(page,header,&pos);

    /*
     * if prev_used() not returned NULL,
     * it means it has found a used block header
     */

    if(header != NULL)
    {
        /* save the current status into the iterator object */
        it->page = (struct sfpool_page*) *header;
        it->header = header;
        it->block_pos = pos;

        return (header + 1);
    }

    /* fill the iterator with zero */
    memset(it,0,sizeof(struct sfpool_it));

    return NULL;
}
