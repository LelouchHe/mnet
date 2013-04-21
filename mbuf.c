#include <stdlib.h>
#include <string.h>

#include "mbuf.h"

//#define DEF_BUF_SIZE 4096
#define DEF_BUF_SIZE 1

struct mb_node_t
{
    struct mb_node_t *next;
    int end;                        // 缓冲区尾,便于计算
    int read;                       // 可读位置
    int write;                      // 可写位置
    char data[0];
};

struct mbuf_t
{
    struct mb_node_t *read_buf;     // 读缓冲
    struct mb_node_t *write_buf;    // 写缓冲
    struct mb_node_t *free_buf;     // 空闲缓冲
    int ini_size;                   // 新缓冲区大小
    int read_size;                  // 当前可读大小
};

// 辅助函数
static int bn_clear(struct mb_node_t *bn)
{
    bn->next = NULL;
    bn->read = 0;
    bn->write = 0;

    return 0;
}

static struct mb_node_t *bn_ini(int size)
{
    struct mb_node_t *bn = (struct mb_node_t *)malloc(sizeof (struct mb_node_t) + size);
    if (bn == NULL)
        return NULL;

    // 缓冲区大小各自管理,不做统一
    // 即同一mbuf下可能有多个不同大小的buf
    bn->end = size;
    bn_clear(bn);
    return bn;
}

static int bn_fini(struct mb_node_t *bn)
{
    free(bn);
    return 0;
}

static int max(int a, int b)
{
    return a > b ? a : b;
}

static int min(int a, int b)
{
    return a > b ? b : a;
}

struct mbuf_t *mb_ini(int ini_size)
{
    struct mbuf_t *mb = (struct mbuf_t *)malloc(sizeof (struct mbuf_t));
    if (mb == NULL)
        return NULL;

    if (ini_size <= DEF_BUF_SIZE)
        ini_size = DEF_BUF_SIZE;

    mb->read_buf = bn_ini(ini_size);
    if (mb->read_buf == NULL)
    {
        free(mb);
        return NULL;
    }

    mb->write_buf = mb->read_buf;
    mb->free_buf = NULL;
    mb->ini_size = ini_size;
    mb->read_size = 0;

    return mb;
}

int mb_fini(struct mbuf_t *mb)
{
    if (mb == NULL)
        return MBUF_NULL_BUF;

    struct mb_node_t *bn = NULL;
    while (mb->read_buf != NULL)
    {
        bn = mb->read_buf;
        mb->read_buf = mb->read_buf->next;
        bn_fini(bn);
    }
    bn = NULL;

    mb->write_buf = NULL;
    while (mb->free_buf != NULL)
    {
        bn = mb->free_buf;
        mb->free_buf = mb->free_buf->next;
        bn_fini(bn);
    }

    mb->ini_size = 0;
    mb->read_size = 0;

    return MBUF_OK;
}

// 可以单纯的进行复制
int mb_swap(struct mbuf_t *mb, struct mbuf_t *a_mb)
{
    if (mb == NULL || a_mb == NULL)
        return MBUF_NULL_BUF;

    if (mb == a_mb)
        return MBUF_SAME_BUF;

    struct mbuf_t t = *mb;
    *mb = *a_mb;
    *a_mb = t;

    return MBUF_OK;
}

int mb_read_size(struct mbuf_t *mb)
{
    if (mb == NULL)
        return MBUF_NULL_BUF;

    return mb->read_size;
}

static struct mb_node_t *get_buf_node(struct mbuf_t *mb)
{
    if (mb->free_buf == NULL)
    {
        mb->free_buf = bn_ini(mb->ini_size);
        if (mb->free_buf == NULL)
            return NULL;
    }

    struct mb_node_t *bn = mb->free_buf;
    mb->free_buf = mb->free_buf->next;

    bn_clear(bn);

    return bn;
}

int mb_append(struct mbuf_t *mb, const char *data, int size)
{
    if (mb == NULL)
        return MBUF_NULL_BUF;

    int data_size = size;
    const char *data_begin = data;
    struct mb_node_t *bn = mb->write_buf;

    while (data_size > 0)
    {
        int write_size = min(bn->end - bn->write, data_size);
        memcpy(bn->data + bn->write, data_begin, write_size);
        data_size -= write_size;
        data_begin += write_size;
        bn->write += write_size;
        mb->read_size += write_size;

        // 写缓冲为空,重新分配
        if (bn->write == bn->end)
        {
            bn = get_buf_node(mb);
            if (bn == NULL)
                return MBUF_NO_MEM;

            mb->write_buf->next = bn;
            mb->write_buf = bn;
        }
    }

    return size;
}

int mb_retrieve(struct mbuf_t *mb, char *data, int *size)
{
    if (mb == NULL)
        return MBUF_NULL_BUF;

    int data_size = *size;
    if (data_size > mb->read_size)
    {
        data_size = mb->read_size;
        *size = data_size;
    }

    char *data_begin = data;
    struct mb_node_t *bn = mb->read_buf;

    while (data_size > 0)
    {
        int read_size = min(bn->write - bn->read, data_size);
        memcpy(data_begin, bn->data + bn->read, read_size);
        data_size -= read_size;
        data_begin += read_size;
        bn->read += read_size;
        mb->read_size -= read_size;

        // 读缓冲耗尽,释放并重新选取
        // 只比较read和write
        // 因为data_size保证了可读取的大小
        if (bn->read == bn->write)
        {
            mb->read_buf = mb->read_buf->next;
            bn->next = mb->free_buf;
            mb->free_buf = bn;
            bn = mb->read_buf;
        }
    }

    return MBUF_OK;
}

// 对部分表进行复制,防止修改
int mb_peek(struct mbuf_t *mb, char *data, int *size)
{
    if (mb == NULL)
        return MBUF_NULL_BUF;

    struct mbuf_t mb_copy = *mb;
    mb = &mb_copy;

    int data_size = *size;
    if (data_size > mb->read_size)
    {
        data_size = mb->read_size;
        *size = data_size;
    }

    char *data_begin = data;
    struct mb_node_t node = *(mb->read_buf);
    struct mb_node_t *bn = &node;

    while (data_size > 0)
    {
        int read_size = min(bn->write - bn->read, data_size);
        memcpy(data_begin, bn->data + bn->read, read_size);
        data_size -= read_size;
        data_begin += read_size;
        bn->read += read_size;
        mb->read_size -= read_size;

        // 只比较read和write
        // 因为data_size保证了可读取的大小
        if (bn->read == bn->write)
        {
            node = *(node.next);
            bn = &node;
        }
    }

    return MBUF_OK;
}

struct mbuf_t * mb_merge(struct mbuf_t *a, struct mbuf_t *b)
{
    if (a == NULL)
        return b;
    if (b == NULL)
        return a;

    a->write_buf->next = b->read_buf;
    a->write_buf = b->write_buf;
    if (a->free_buf == NULL)
        a->free_buf = b->free_buf;
    else
        a->free_buf->next = b->free_buf;

    if (a->ini_size < b->ini_size)
        a->ini_size = b->ini_size;
    a->read_size += b->read_size;

    mb_fini(b);

    return a;
}

