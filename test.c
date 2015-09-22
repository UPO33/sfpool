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
    printf("SIG\n");
    sfpool_dump(pool);
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
            printf("ALLOC   :   %p\n",*p);
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
        printf("FREE   :   %p\n",*p);
        *p = NULL;
    }
}

void (*func[2]) (void) = { func1,func2 };

int main2 (void)
{
    signal(SIGINT,sighandler);
    memset(ptrs,0,sizeof(ptrs));

    pool = sfpool_create (1,512,SFPOOL_EXPAND_FACTOR_ONE);

    srand(clock());

    while(1)
    {
        func[rand() % 2]();
    }
    
    sfpool_dump(pool);
    return 0;
}

int main (void)
{
    signal(SIGINT,sighandler);
    memset(ptrs,0,sizeof(ptrs));

    pool = sfpool_create (1,8,SFPOOL_EXPAND_FACTOR_ONE);

    srand(clock());

    for(int i = 0;i < 9;i++)
    {
        ptrs[i] = sfpool_alloc(pool);
        *((size_t*) ptrs[i]) = i;
    }

    sfpool_free(pool,ptrs[0]);
    sfpool_free(pool,ptrs[2]);

    struct sfpool_it it;
    void* b = sfpool_it_init(pool,&it,NULL);
    while(b)
    {
        printf("CUR -> 0x%X\n",b);
        getchar();
        b = sfpool_it_next(&it);
    }

    sfpool_dump(pool);
    return 0;
}
