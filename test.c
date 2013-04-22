#include <stdio.h>
#include <unistd.h>

#include "mthread_pool.h"

#define MAX_BUF_SIZE 1024

void *fun(void *args)
{
    int id = (int)args;
    printf("in thread #%d\n", id);
    sleep(1);
    return NULL;
}

void *fun_a(void *args)
{
    int id = (int)args;
    printf("in slow thread #%d\n", id);
    sleep(30);
    printf("after sleep\n");
    return NULL;
}

int main()
{
    struct mthread_pool_t *mtp  = mtp_ini();
    int i = 0;
    mtp_run(mtp, fun_a, i);
    for (i = 1; i < 20; i++)
        mtp_run(mtp, fun, i);

    mtp_start(mtp, 4);

    sleep(30);

    mtp_fini(mtp);

    return 0;
}
