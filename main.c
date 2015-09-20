#include "sfpool.h"
#include <stdio.h>
#include <signal.h>

static void sighandler (int signal)
{
    exit(0);
}

int main (void)
{
    signal(SIGTERM,sighandler);

    printf("SSS\n");
    size_t num = 8,al = 8;

    while(1)
    {
        scanf("%d %d",&num,&al);
        printf("2) %u --[~%u]--> %u\n",num,al,round_size2(num,al));
        printf("1) %u --[~%u]--> %u\n",num,al,round_size(num,al));
    }

    return 0;
}
