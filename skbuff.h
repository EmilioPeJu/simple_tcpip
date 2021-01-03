#ifndef SK_BUFF_H
#define SK_BUFF_H
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include "list.h"
#include "address.h"
struct ethernet_hdr;
struct intf;
struct ip_hdr;
struct tcp_hdr;
struct udp_hdr;

struct sk_buff {
    struct list_head list;
    struct intf *intf;
    struct ethernet_hdr *ethernet_hdr;
    struct ip_hdr *ip_hdr;
    struct tcp_hdr *tcp_hdr;
    struct udp_hdr *udp_hdr;
    size_t len;
    u8 *data;
    u8 *tail;
    u8 *end;
    // variable size field, must be the last one
    u8 head[1];
};

inline struct sk_buff *alloc_skb(size_t size)
{
    struct sk_buff *skb = calloc(1, size + sizeof(struct sk_buff) - 1);
    skb->data = skb->tail = skb->head;
    skb->end = skb->head + size;
    INIT_LIST_HEAD(&skb->list);
    return skb;
}

inline void free_skb(struct sk_buff *skb)
{
    free(skb);
}

inline void skb_reserve(struct sk_buff *skb, size_t size)
{
    skb->data += size;
    skb->tail += size;
    assert(skb->tail <= skb->end);
}

inline void *skb_push(struct sk_buff *skb, size_t size)
{
    skb->data -= size;
    skb->len += size;
    assert(skb->data >= skb->head);
    return skb->data;
}

inline void *skb_pull(struct sk_buff *skb, size_t size)
{
    skb->data += size;
    skb->len -= size;
    assert(skb->data <= skb->tail);
    return skb->data;
}

inline void *skb_put(struct sk_buff *skb, size_t size)
{
    void *tmp = skb->tail;
    skb->tail += size;
    skb->len += size;
    assert(skb->tail <= skb->end);
    return tmp;
}

inline size_t skb_headroom(struct sk_buff *skb)
{
    return (size_t) (skb->data - skb->head);
}

inline size_t skb_tailroom(struct sk_buff *skb)
{
    return (size_t) (skb->end - skb->tail);
}

#endif
