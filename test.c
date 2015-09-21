#include "sfpool.h"
#include <stdio.h>
#include <signal.h>

static struct sfpool* pool = NULL;
static void* ptrs[1024];

static void sighandler (int signal)
{
    exit(0);
}

static int rnd (void)
{

}

static void** find_slot_free (void)
{
    for(size_t i = 0;i < 1024;i++)
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
    for(size_t i = 0;i < 1024;i++)
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
    printf("ALLOC 1\n");
    for(int i = 0;i < 1;i++)
    {
        void** p = find_slot_free();
        if(p)
        {
            *p = sfpool_alloc(pool);
        }
    }
}

static void func2 (void)
{
    printf("ALLOC 2\n");
    for(int i = 0;i < 2;i++)
    {
        void** p = find_slot_free();
        if(p)
        {
            *p = sfpool_alloc(pool);
        }
    }
}

static void func3 (void)
{
    printf("ALLOC 3\n");
    for(int i = 0;i < 3;i++)
    {
        void** p = find_slot_free();
        if(p)
        {
            *p = sfpool_alloc(pool);
        }
    }
}

static void func4 (void)
{
    printf("FREE 1\n");
    void** p = find_slot_used();
    if(p)
    {
        sfpool_free(pool,*p);
        *p = NULL;
    }
}

static void func5 (void)
{
    printf("FREE 5\n");
    for(size_t i = 0; i < 5;i ++)
    {
        void** p = find_slot_used();
        if(p)
        {
            sfpool_free(pool,*p);
            *p = NULL;
        }
    }
}

void (*func[5]) (void) = { func1,func2,func3,func4,func5 };

int main (void)
{
    signal(SIGTERM,sighandler);
    memset(ptrs,0,sizeof(ptrs));

    pool = sfpool_create (17,128,SFPOOL_EXPAND_FACTOR_ONE);

    srand(clock());

    while(1)
    {
        func[rand() % 4]();
    }

    return 0;
}
