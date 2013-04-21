#ifndef _MNET_MTHREAD_H
#define _MENT_MTHREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#define MTHREAD_OK 0
#define MTHREAD_NULL_THREAD -1
#define MTHREAD_STARTED -2
#define MTHREAD_NOT_STARTED -2

struct mthread_t;

struct mthread_t *mt_ini(void *(*fun)(void *));
int mt_fini(struct mthread_t *mt);

int mt_start(struct mthread_t *mt, void *args);
int mt_join(struct mthread_t *mt);

int mt_tid(struct mthread_t *mt);

#ifdef __cplusplus
}
#endif

#endif
