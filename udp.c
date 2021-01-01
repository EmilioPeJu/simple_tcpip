#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "graph.h"
#include "ip.h"
#include "net.h"
#include "skbuff.h"
#include "udp.h"
#include "utils.h"

static struct udp_sock *fd_to_sock[MAX_UDP_SOCKS];

static inline bool is_good_udp_sock(int sockfd)
{
    if (sockfd < 0 || sockfd >= MAX_UDP_SOCKS || !fd_to_sock[sockfd])
        return false;
    return true;
}

bool udp_input(struct sk_buff *skb)
{
    struct udp_hdr *hdr = (struct udp_hdr *) skb->data;
    struct udp_sock *sock = udp_socks_manager_find(
        IF_UDP_SOCKS_MANAGER(skb->intf), NTOHS(hdr->dst_port));
    skb->udp_hdr = hdr;
    if (!sock)
        return false;
    skb_pull(skb, UDP_HDR_SIZE);
    list_add(&skb->list, &sock->rx_skb);
    return true;
}

bool udp_out(struct udp_sock *sock, char *buff, size_t size, struct ip_addr ip,
             u16 port)
{
    struct sk_buff *skb = alloc_skb(BUFF_HEADROOM + BUFF_TAILROOM + \
        UDP_HDR_SIZE + size);
    skb_reserve(skb, BUFF_HEADROOM);
    struct udp_hdr *hdr = (struct udp_hdr *) skb_put(skb, UDP_HDR_SIZE);
    memcpy(skb_put(skb, size), buff, size);
    hdr->src_port = HTONS(sock->local_port);
    hdr->dst_port = HTONS(port);
    hdr->total_len = HTONS(skb->len);
    skb->intf = sock->intf;
    return ip_output(sock->intf->node, ip, IP_PROTO_UDP, skb);
}

int udp_socket(struct node *node)
{
    for (size_t i=0; i < MAX_UDP_SOCKS; i++) {
        if (!fd_to_sock[i]) {
            struct udp_sock *sock = calloc(1, sizeof(struct udp_sock));
            sock->node = node;
            INIT_LIST_HEAD(&sock->rx_skb);
            fd_to_sock[i] = sock;
            return i;
        }
    }
    return -1;
}

int udp_close(int sockfd)
{
    if (!is_good_udp_sock(sockfd))
        return -1;
    struct udp_sock *sock = fd_to_sock[sockfd];
    fd_to_sock[sockfd] = NULL;
    if (sock->bound)
        list_del(&sock->list);
    free(sock);
    return 0;
}

int udp_bind(int sockfd, struct ip_addr ip, u16 port)
{
    if (!is_good_udp_sock(sockfd) || fd_to_sock[sockfd]->bound)
        return -1;
    struct udp_sock *sock = fd_to_sock[sockfd];
    struct intf *intf = _get_matching_interface(sock->node, ip);
    if (!intf)
        return -1;
    sock->bound = true;
    sock->intf = intf;
    sock->local_port = port;
    list_add(&sock->list, &IF_UDP_SOCKS_MANAGER(intf)->socks);
    return 0;
}

ssize_t udp_recvfrom(int sockfd, char *buff, size_t size, struct ip_addr *ip,
                     u16 *port)
{
    if (!is_good_udp_sock(sockfd) || !fd_to_sock[sockfd]->bound)
        return -1;
    struct udp_sock *sock = fd_to_sock[sockfd];
    if (!list_empty(&sock->rx_skb)) {
        struct sk_buff *skb = list_last_entry(&sock->rx_skb,
                                              struct sk_buff,
                                              list);
        ssize_t final_size = MIN(skb->len, size);
        memcpy(buff, skb->data, final_size);
        skb_pull(skb, final_size);
        if (ip)
            ip->iaddr = skb->ip_hdr->src_ip.iaddr;
        if (port)
            *port = NTOHS(skb->udp_hdr->src_port);
        if (!skb->len) {
            list_del(&skb->list);
            free_skb(skb);
        }
        return final_size;
    }
    return -1;
}

ssize_t udp_sendto(int sockfd, char *buff, size_t size, struct ip_addr ip,
                   u16 port)
{
    if (!is_good_udp_sock(sockfd) || !fd_to_sock[sockfd]->bound)
        return -1;
    struct udp_sock *sock = fd_to_sock[sockfd];
    ssize_t final_size = MIN(size, MAX_UDP_PAYLOAD);
    bool result = udp_out(sock, buff, final_size, ip, port);
    if (!result)
        return -1;
    return final_size;
}

void init_udp_socks_manager(struct udp_socks_manager *manager)
{
    INIT_LIST_HEAD(&manager->socks);
}

struct udp_sock *udp_socks_manager_find(struct udp_socks_manager *manager,
                                        u16 port)
{
    struct udp_sock *sock;
    list_for_each_entry(sock, &manager->socks, list) {
        if (sock->local_port == port)
            return sock;
    }
    return NULL;
}

void dump_udp_socks_manager(struct udp_socks_manager *manager)
{
    struct udp_sock *sock;
    printf("UDP Sockets: ");
    list_for_each_entry(sock, &manager->socks, list) {
        printf("%hu ", sock->local_port);
    }
    printf("\n");
}
