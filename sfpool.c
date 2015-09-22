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

struct sfpool* sfpool_create (size_t item_size,size_t page_size,enum SFPOOL_EXPAND_FACTOR expand_factor)
{
    /* reserve a memory block for the pool object */

    struct sfpool* pool = malloc(sizeof(struct sfpool));

    if(pool == NULL)
    {
        return NULL;
    }

    memset(pool,0,sizeof(struct sfpool));

    /*
     * round the size of each item according to system word size and put
     * some extra bytes (paddings) in order to make each item start
     * on word sized boundary address. this will result in higher speed
     * performance. but on the other hand it wastes memory as well.
     */

    pool->item_size = round_size(item_size);
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

    it = pool->pages;

    while(it != NULL)
    {
        prev = it->prev;
        free(it);
        it = prev;
    }
}

static struct sfpool_page* add_page (struct sfpool* pool,size_t page_size)
{
    printf("ADD_PAGE() : %u\n",page_size);
    size_t raw_size = ((WORD_SIZE + pool->item_size) * page_size) +
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

    page->item_count = item_count;
    page->free_count = item_count;

    pool->all_pages = page;
    pool->free_pages = page;

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

    for(size_t i = 0;i < (page_size - 1);i++)
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
    printf("DELTE_PAGE() : %u\n",page->item_count);
    size_t item_count = page->item_count;

    if(page->prev) page->prev->next = page->next;
    if(page->next) page->next->prev = page->prev;

    /* if the first page is the page itself */

    if(pool->pages == page)
    {
        pool->pages = page->prev;
    }

    /* if the page is the working page */

    if(pool->working_page == page)
    {
        /* look through all pages for a page which has free items */

        struct sfpool_page* it = pool->pages;

        while(it != NULL)
        {
            /* this page has free items */

            if(it->free_count != 0)
            {
                /* set the page as our working page */

                pool->working_page = it;
                break;
            }

            it = it->prev;
        }
    }

    pool->item_count -= item_count;
    pool->page_count--;

    /* if this was the last page */

    if(pool->page_count == 0)
    {
        pool->item_count = item_count;
        return;
    }
}

void* sfpool_alloc (struct sfpool* pool)
{
    /* get the current working page */

    struct sfpool_page* wp = pool->working_page;

    /*
     * if we already have a working page. note that a working page
     * always has at least one free item !
     */

    if(wp != NULL)
    {
        /* get the address of the first free item */

        size_t* item = (size_t*) wp->free_first;

        /*
         * now the first free item will be marked as used,
         * so put the next free item as the new first free item.
         */

        wp->free_first = (size_t*) *item;
        wp->free_count--;
        printf("page [%p] : free [%u]\n",wp,wp->free_count);
    
        /* 
         * put the address of the page in the header of the item.
         * this will be useful when we want to free an item.
         */

        *item = (size_t) wp;
    
        /* check if this page has become full or not */

        if(wp->free_count == 0)
        {
            /* [HINT] we need a better algorithm */

            /* look through all existing pages for free item */

            wp = pool->pages;

            while(wp != NULL)
            {
                /* that's it! a page with free item(s) ! */

                if(wp->free_count != 0)
                {
                    printf("JUNK PAGE !!!!!!!!!1\n");
                    /* set the page as our working page */

                    pool->working_page = wp;
                    break;
                }

                wp = wp->prev;
            }

            /* all pages are full, we need a new one */

            if(wp == NULL)
            {
                size_t item_count;
                
                /* how big the new page should be ? */

                switch(pool->expand_factor)
                {
                    /*
                     * a new page as big as sum of
                     * all existing pages together
                     */

                    case SFPOOL_EXPAND_FACTOR_ONE:
                        item_count = pool->item_count;
                        break;

                    /* 
                     * a new page 2 times bigger than sum of
                     * all existing pages together
                     */

                    case SFPOOL_EXPAND_FACTOR_TWO:
                        item_count = pool->item_count * 2;
                        break;

                    default:
                        item_count = pool->item_count;
                }

                /* request a new page and set it as our working page */

                pool->working_page = add_page(pool,item_count);

                /* if the requested page could not be created for any reason */

                if(pool->working_page == NULL)
                {
                    return NULL;
                }
            }
        }

        /* the item lives just a word size after the header :) */
        
        return (void*) (item + 1);
    }

    printf("page no WP! ----------------------------------\n");
    /*
     * we don't have a working page yet. this only happens when
     *
     *  - we have no pages
     *  - this is the first time of calling sfpool_alloc()
     *  - all pages are full and add_page() failed.
     */

    pool->working_page = add_page(pool,pool->item_count);
    printf("item_count : %u\n",pool->item_count);

    /* if the requested page could not be created for any reason */

    if(pool->working_page == NULL)
    {
        return NULL;
    }

    return sfpool_alloc(pool);
}

void sfpool_free (struct sfpool* pool,void* ptr)
{
    /* header lives just a word size before the item */

    size_t* header = ((size_t*) (ptr)) - 1;

    /* header's data is a address to the owner page */

    struct sfpool_page* page = (struct sfpool_page*) *header;

    /*
     * make this item free by inserting the address of other free item in it.
     * then put this item as our new free item.
     */

    *header = (size_t) page->free_first;
    page->free_first = header;
    page->free_count++;

    /* if the owner page is entirely free */

    if(page->free_count == page->item_count)
    {
        /* if the owner page is not the working page, then delete it */

        delete_page(pool,page);
    }
}

void sfpool_dump (struct sfpool* pool)
{
    printf(
    "== SFPOOL ==\n"
    "item_size      : %u\n"
    "item_count     : %u\n"
    "page_count     : %u\n"
    "expand_factor  : %u\n",
    pool->item_size,
    pool->item_count,
    pool->page_count,
    pool->expand_factor);
}
