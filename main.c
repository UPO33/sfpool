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

    return 0;
}
