#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#include "mthread.h"
#include "mthread_pool.h"

typedef void *(*task_fun_t)(void *);

struct task_node_t
{
    struct task_node_t *next;
    task_fun_t task;
    void *args;
};

struct task_queue_t
{
    struct task_node_t *work_task_head; 
    struct task_node_t *work_task_tail;
    struct task_node_t *free_task;
    int num;

};

static int tn_clear(struct task_node_t *tn)
{
    assert(tn != NULL);

    tn->next = NULL;
    tn->task = NULL;
    tn->args = NULL;

    return 0;
}

static int tq_ini(struct task_queue_t *tq)
{
    assert(tq != NULL);

    tq->work_task_head = NULL;
    tq->work_task_tail = NULL;
    tq->free_task = NULL;
    tq->num = 0;

    return 0;
}

static int tq_fini(struct task_queue_t *tq)
{
    assert(tq != NULL);

    struct task_node_t *tn = NULL;

    tn = tq->work_task_head;
    while (tn != NULL)
    {
        tq->work_task_head = tq->work_task_head->next;
        free(tn);
        tn = tq->work_task_head;
    }
    tq->work_task_tail = NULL;

    tn = tq->free_task;
    while (tn != NULL)
    {
        tq->free_task = tq->free_task->next;
        free(tn);
        tn = tq->free_task;
    }

    tq->num = 0;

    return 0;
}

static struct task_node_t *get_task_node(struct task_queue_t *tq)
{
    if (tq->free_task == NULL)
    {
        tq->free_task = (struct task_node_t *)malloc(sizeof (struct task_node_t));
        if (tq->free_task == NULL)
            return NULL;

        tq->free_task->next = NULL;
    }

    struct task_node_t *tn = tq->free_task;
    tq->free_task = tq->free_task->next;

    tn_clear(tn);

    return tn;
}

static int tq_put(struct task_queue_t *tq, task_fun_t task, void *args)
{
    assert(tq != NULL);

    struct task_node_t *tn = NULL;

    tn = get_task_node(tq);
    tn->task = task;
    tn->args = args;

    if (tq->work_task_head == NULL)
    {
        tq->work_task_head = tn;
        tq->work_task_tail = tn;
    }
    else
    {
        tq->work_task_tail->next = tn;
        tq->work_task_tail = tn;
    }

    tq->num++;

    return 0;
}

static struct task_node_t tq_get(struct task_queue_t *tq)
{
    assert(tq != NULL);

    struct task_node_t tn;
    tn_clear(&tn);
    if (tq->num == 0)
        return tn;

    tn = *(tq->work_task_head);
    tq->work_task_head = tq->work_task_head->next;
    if (tq->work_task_head == NULL)
        tq->work_task_tail = NULL;
    tq->num--;

    tn.next = NULL;
    return tn;
}

struct mthread_pool_t
{
    struct mthread_t **mts;
    int num;

    int run;
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    struct task_queue_t tq;
};

struct mthread_pool_t *mtp_ini()
{
    struct mthread_pool_t *mtp = (struct mthread_pool_t *)malloc(sizeof (struct mthread_pool_t));
    if (mtp == NULL)
        return NULL;

    mtp->mts = NULL;
    mtp->num = 0;

    mtp->run = 0;
    pthread_mutex_init(&mtp->mutex, NULL);
    pthread_cond_init(&mtp->cond, NULL);

    tq_ini(&mtp->tq);

    return mtp;
}

int mtp_fini(struct mthread_pool_t *mtp)
{
    if (mtp == NULL)
        return MTP_NULL_POOL;

    if(mtp->run)
        mtp_stop(mtp);

    if (mtp->mts != NULL)
    {
        int i;
        for (i = 0; i < mtp->num; i++)
            mt_fini(mtp->mts[i]);
        free(mtp->mts);
    }

    pthread_cond_destroy(&mtp->cond);
    pthread_mutex_destroy(&mtp->mutex);

    tq_fini(&mtp->tq);

    return MTP_OK;
}

static struct task_node_t take(struct mthread_pool_t *mtp)
{
    assert(mtp != NULL);

    pthread_mutex_lock(&mtp->mutex);

    while (mtp->tq.num == 0 && mtp->run)
        pthread_cond_wait(&mtp->cond, &mtp->mutex);

    struct task_node_t tn = tq_get(&mtp->tq);
    pthread_mutex_unlock(&mtp->mutex);

    return tn;
}

static void *thread_fun(void *args)
{
    assert(args != NULL);

    struct mthread_pool_t *mtp = (struct mthread_pool_t *)args;
    while (mtp->run)
    {
        struct task_node_t tn = take(mtp);
        if (tn.task != NULL)
            tn.task(tn.args); // 忽略返回值,其实可以变更返回类型
    }

    return NULL;
}

int mtp_start(struct mthread_pool_t *mtp, int num)
{
    if (mtp == NULL)
        return MTP_NULL_POOL;

    if (mtp->run)
        mtp_stop(mtp);

    mtp->mts = (struct mthread_t **)malloc(num * sizeof (struct mthread_t *));
    if (mtp->mts == NULL)
        return MTP_MEM_ERROR;
    mtp->num = num;
    mtp->run = 1;

    int i;
    for (i = 0; i < mtp->num; i++)
    {
        mtp->mts[i] = mt_ini(thread_fun);
        assert(mtp->mts[i] != NULL);
        mt_start(mtp->mts[i], mtp);
    }

    return MTP_OK;
}

int mtp_stop(struct mthread_pool_t *mtp)
{
    pthread_mutex_lock(&mtp->mutex);
    mtp->run = 0;
    pthread_mutex_unlock(&mtp->mutex);
    pthread_cond_broadcast(&mtp->cond);

    int i;
    for (i = 0; i < mtp->num; i++)
        mt_join(mtp->mts[i]);

    return MTP_OK;
}

int mtp_run(struct mthread_pool_t *mtp, void *(*fun)(void *), void *args)
{
    if (mtp == NULL)
        return MTP_NULL_POOL;

    if (mtp->tq.num == 0 && mtp->run)
        fun(args);
    else
    {
        pthread_mutex_lock(&mtp->mutex);
        tq_put(&mtp->tq, fun, args);
        pthread_mutex_unlock(&mtp->mutex);
        pthread_cond_signal(&mtp->cond);
    }

    return 0;
}

