#include "sfpool.h"
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

#define SIZE (10 * 1024)
static struct sfpool* pool = NULL;
static void* ptrs[SIZE];

static void sighandler (int signal)
{
    sfpool_dump(pool);
    printf("now free everything !\n");
    struct sfpool_it it;
    void* ptr = sfpool_it_first(pool,&it);
    while(ptr)
    {
        sfpool_free(pool,ptr);
        ptr = sfpool_it_next(&it);
    }

    sfpool_dump(pool);
	
	//ghjk
    exit(0);
}

static void** find_slot_free (void)
{
    for(size_t i = 0;i < SIZE;i++)
    {
        if(ptrs[i] == NULL)
        {
            return &ptrs[i];
        }
    }

    return NULL;
}

static void** find_slot_used (void)
{
    for(size_t i = 0;i < SIZE;i++)
    {
        if(ptrs[i] != 0)
        {
            return &ptrs[i];
        }
    }

    return NULL;
}

static void func1 (void)
{
    for(int i = 0;i < 1;i++)
    {
        void** p = find_slot_free();
        if(p)
        {
            *p = sfpool_alloc(pool);
            printf("ALLOC: %p\n",*p);
            if(*p == NULL)
            {
                sleep(5000);
            }
        }
    }
}

static void func2 (void)
{
    void** p = find_slot_used();
    if(p)
    {
        sfpool_free(pool,*p);
        printf("FREE : %p\n",*p);
        *p = NULL;
    }
}

void (*func[2]) (void) = { func1,func2 };

int main (void)
{
    signal(SIGINT,sighandler);
    memset(ptrs,0,sizeof(ptrs));

    pool = sfpool_create (1,32,SFPOOL_EXPAND_TYPE_ONE);

    srand(clock());

    while(1)
    {
        func[rand() % 2]();
        printf("PAGE : %lu\n",pool->page_count);
    }
    
    sfpool_dump(pool);
    return 0;
}

int main2 (void)
{
    signal(SIGINT,sighandler);
    memset(ptrs,0,sizeof(ptrs));

    pool = sfpool_create (1,8,SFPOOL_EXPAND_TYPE_ONE);

    for(int i = 0;i < 9;i++)
    {
        ptrs[i] = sfpool_alloc(pool);
        *((size_t*) ptrs[i]) = i;
    }

    struct sfpool_it it;
    size_t* b;

    b = sfpool_it_first(pool,&it);
    while(b)
    {   
        printf("[%p] (%u) = %X\n",it.page,it.block_pos,*b);
        b = sfpool_it_next(&it);
    }

    b = sfpool_it_block(pool,&it,ptrs[8]);
    while(b)
    {   
        printf("[%p] (%u) = %X\n",it.page,it.block_pos,*b);
        b = sfpool_it_prev(&it);
    }

    sfpool_dump(pool);
    return 0;
}
