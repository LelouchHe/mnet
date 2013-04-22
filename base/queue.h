#ifndef MNET_BASE_QUEUE_H
#define MNET_BASE_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

struct queue_node_head_t
{
    // 目前是单链表
    //struct queue_node_head_t *prev;
    struct queue_node_head_t *next;
};

struct queue_head_t
{
    struct queue_node_head_t 
};

#ifdef __cplusplus
}
#endif

#endif
