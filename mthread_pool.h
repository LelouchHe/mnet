#ifndef _MNET_MTHREAD_POOL_H
#define _MNET_MTHREAD_POOL_H

#ifdef __cplusplus
extern "C" {
#endif

#define MTP_OK 0
#define MTP_NULL_POOL -1
#define MTP_MEM_ERROR -2

struct mthread_pool_t;

struct mthread_pool_t *mtp_ini();
int mtp_fini(struct mthread_pool_t *mtp);

int mtp_start(struct mthread_pool_t *mtp, int num);
int mtp_stop(struct mthread_pool_t *mtp);

int mtp_run(struct mthread_pool_t *mtp, void *(*fun)(void *), void *args);

#ifdef __cplusplus
}
#endif

#endif
