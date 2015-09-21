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
     * round the size of each item according to system word size and put
     * some extra bytes (paddings) in order to make each item start
     * on word sized boundary address. this will result in higher speed
     * performance. but on the other hand it wastes memory as well.
     */
    
    pool->item_size = round_size(item_size);

    /* check the previous pages and find out how big this page should be ? */

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

static struct sfpool_page* add_page (struct sfpool* pool,size_t item_count)
{
    size_t raw_size = ((WORD_SIZE + pool->item_size) * item_count) +
                      sizeof(struct sfpool_page);

    struct sfpool_page* page = (struct sfpool_page*) malloc(raw_size);

    if(page == NULL)
    {
        return NULL;
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

    return page;
}

static void delete_page (struct sfpool* pool,struct sfpool_page* page)
{
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

                /* set the page as our working page */

                pool->working_page = add_page(pool,item_count);
            }
        }

        /* the item lives just a word size after the header :) */

        return (void*) (item + 1);
    }

    /*
     * we don't have a working page yet. this only happens when
     * we have no pages, this is the first time of calling sfpool_alloc().
     */

    /*
     * this is our first page, so we don't need
     * to consider the expand factor.
     */

    pool->working_page = add_page(pool,pool->item_count);

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
