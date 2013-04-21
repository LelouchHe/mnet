#include <stdlib.h>
#include <pthread.h>

#include "mthread.h"

typedef void *(*thread_fun_t)(void *);

struct mthread_t
{
    pthread_t tid;
    int joined;
    thread_fun_t fun;
};

struct mthread_t *mt_ini(thread_fun_t fun)
{
    struct mthread_t *mt = (struct mthread_t *)malloc(sizeof (struct mthread_t));
    if (mt == NULL)
        return NULL;

    mt->tid = 0;
    mt->fun = fun;
    mt->joined = 0;

    return mt;
}

int mt_fini(struct mthread_t *mt)
{
    if (mt == NULL)
        return MTHREAD_NULL_THREAD;

    if (mt->tid != 0 && mt->joined == 0)
        pthread_detach(mt->tid);
    mt->tid = 0;
    mt->fun = NULL;
    mt->joined = 1;

    return MTHREAD_OK;
}

int mt_start(struct mthread_t *mt, void *args)
{
    if (mt == NULL)
        return MTHREAD_NULL_THREAD;
    if (mt->tid != 0)
        return MTHREAD_STARTED;

    pthread_create(&mt->tid, NULL, mt->fun, args);

    return MTHREAD_OK;
}

int mt_join(struct mthread_t *mt)
{
    if (mt == NULL)
        return MTHREAD_NULL_THREAD;
    if (mt->tid == 0)
        return MTHREAD_NOT_STARTED;

    pthread_join(mt->tid, NULL);
    mt->joined = 1;

    return MTHREAD_OK;
}

int mt_tid(struct mthread_t *mt)
{
    return mt->tid;
}
