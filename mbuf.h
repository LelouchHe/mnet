#ifndef _MNET_MBUF_H
#define _MNET_MBUF_H

#ifdef __cplusplus
extern "C" {
#endif

#define MBUF_OK 0
#define MBUF_NULL_BUF -1
#define MBUF_SAME_BUF -2
#define MBUF_NO_MEM -3
#define MBUF_EMPTY_BUF -4

struct mbuf_t;

struct mbuf_t *mb_ini(int ini_size);
int mb_fini(struct mbuf_t *mb);

int mb_swap(struct mbuf_t *mb, struct mbuf_t *a_buf);

int mb_read_size(struct mbuf_t *mb);

int mb_append(struct mbuf_t *mb, const char *data, int size);
int mb_retrieve(struct mbuf_t *mb, char *data, int *size);
int mb_peek(struct mbuf_t *mb, char *data, int *size);

// merge后不能直接使用a/b
// 必须使用返回值
struct mbuf_t * mb_merge(struct mbuf_t *a, struct mbuf_t *b);

#ifdef __cplusplus
}
#endif

#endif
