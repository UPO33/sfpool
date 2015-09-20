#include "sfpool.h"

#define WORD_SIZE ((size_t) - 1)

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

    return pool;
}

static int add_page (struct sfpool* pool)
{
    size_t raw_size = ((WORD_SIZE + pool->item_size) * pool->item_count) + sizeof(struct sfpool_page);
    unsigned char* raw_data = malloc(raw_size);
}
