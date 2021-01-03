#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "graph.h"
#include "ip.h"
#include "net.h"
#include "skbuff.h"
#include "tcp.h"
#include "utils.h"

static struct tcp_sock *fd_to_sock[MAX_TCP_SOCKS];
static struct tcp_connection_sock *fd_conn_to_sock[MAX_TCP_CONN_SOCKS];

static bool close_connection(struct tcp_connection_sock *conn,
                             struct sk_buff *skb);

static bool handle_estab(struct tcp_connection_sock *conn,
                         struct sk_buff *skb);

static bool handle_syn_sent(struct tcp_connection_sock *conn,
                            struct sk_buff *skb);

u16 calc_tcp_cksum(struct tcp_hdr *hdr, size_t size,
                   struct ip_addr src_ip, struct ip_addr dst_ip)
{
    // we assume we have 12 bytes available before the start of the header
    // which is true because it's inside a sk_buff with extra head room
    struct pseudo_ip_hdr *pip_hdr = (struct pseudo_ip_hdr *) \
        ((char *) hdr - PSEUDO_IP_HDR_SIZE);
    char backup[PSEUDO_IP_HDR_SIZE];
    memcpy(backup, pip_hdr, PSEUDO_IP_HDR_SIZE);
    pip_hdr->src_ip.iaddr = src_ip.iaddr;
    pip_hdr->dst_ip.iaddr =  dst_ip.iaddr;
    pip_hdr->rsrv = 0;
    pip_hdr->proto = IP_PROTO_TCP;
    pip_hdr->len = HTONS(size);
    u16 result = calc_checksum_16((char *) hdr - PSEUDO_IP_HDR_SIZE,
                                  size + PSEUDO_IP_HDR_SIZE);
    memcpy(pip_hdr, backup, PSEUDO_IP_HDR_SIZE);
    return result;
}

static void fill_tcp_cksum(struct tcp_hdr *hdr, size_t size,
                           struct ip_addr src_ip, struct ip_addr dst_ip)
{
    hdr->cksum = HTONS(calc_tcp_cksum(hdr, size, src_ip, dst_ip));
}

static bool check_tcp_cksum(struct tcp_hdr *hdr, size_t size,
                            struct ip_addr src_ip, struct ip_addr dst_ip)
{
    return calc_tcp_cksum(hdr, size, src_ip, dst_ip) == 0;
}

static inline bool is_good_tcp_sock(int sockfd)
{
    if (sockfd < 0 || sockfd >= MAX_TCP_SOCKS || !fd_to_sock[sockfd])
        return false;
    return true;
}

static inline bool is_good_tcp_connection_sock(int sockfd)
{
    if (sockfd < 0 || sockfd >= MAX_TCP_CONN_SOCKS || !fd_conn_to_sock[sockfd])
        return false;
    return true;
}

static inline struct tcp_sock *tcp_socks_manager_find(
    struct tcp_socks_manager *manager, u16 port)
{
    struct tcp_sock *sock;
    list_for_each_entry(sock, &manager->socks, list) {
        if (sock->local_port == port)
            return sock;
    }
    return NULL;
}

static inline struct tcp_connection_sock *find_matching_connection(
    struct tcp_sock *sock, struct ip_addr remote_ip, u16 remote_port)
{
    struct tcp_connection_sock *conn;
    list_for_each_entry(conn, &sock->conns, list) {
        if (conn->remote_port == remote_port
                && conn->remote_ip.iaddr == remote_ip.iaddr)
            return conn;
    }
    return NULL;
}

static bool close_connection(struct tcp_connection_sock *conn,
                             struct sk_buff *skb)
{
    struct tcp_flags flags;
    printf("Closing connection\n");
    // close connection the rude way
    flags.val = 0;
    flags.ack = 1;
    flags.rst = 1;
    tcp_out(conn, NULL, 0, flags);
    list_del(&conn->list);
    list_add(&conn->list, &conn->parent->closed_conns);
    return true;
}

static bool handle_estab(struct tcp_connection_sock *conn,
                         struct sk_buff *skb)
{
    struct tcp_hdr *hdr = (struct tcp_hdr *) skb->data;
    if (NTOHL(hdr->seq) == conn->rcv.nxt) {
        if (hdr->flags.rst) {
            return close_connection(conn, skb);
        } else if (hdr->flags.fin) {
            conn->rcv.nxt += 1;
            return close_connection(conn, skb);
        } else {
            skb_pull(skb, TCP_HDR_SIZE);
            conn->rcv.nxt += skb->len;
            list_add(&skb->list, &conn->rx_skb);
            return true;
        }
    }
    return false;
}

static bool handle_syn_sent(struct tcp_connection_sock *conn,
                            struct sk_buff *skb)
{
    struct tcp_hdr *hdr = (struct tcp_hdr *) skb->data;
    if (hdr->flags.ack && !hdr->flags.syn
            && NTOHL(hdr->seq) == conn->rcv.nxt) {
        conn->state = handle_estab;
    }
    return false;
}

static bool handle_new_connection(struct tcp_sock *sock,
                                  struct sk_buff *skb)
{
    struct tcp_flags flags;
    printf("Got new connection\n");
    struct tcp_hdr *hdr = (struct tcp_hdr *) skb->data;
    struct tcp_connection_sock *conn = \
        calloc(1, sizeof(struct tcp_connection_sock));
    INIT_LIST_HEAD(&conn->rx_skb);
    INIT_LIST_HEAD(&conn->tx_skb);
    conn->remote_ip.iaddr = skb->ip_hdr->src_ip.iaddr;
    conn->remote_port = NTOHS(hdr->src_port);
    conn->parent = sock;
    conn->snt.una = 0;
    conn->snt.nxt = 0;
    conn->snt.wnd = NTOHS(hdr->wnd_sz);
    // TODO: use a better initial sequence number
    conn->snt.iss = 0;
    conn->rcv.nxt = NTOHL(hdr->seq) + 1;
    conn->rcv.wnd = TCP_WINDOW_SIZE;
    conn->rcv.irs = hdr->seq;
    conn->state = handle_syn_sent;
    list_add(&conn->list, &sock->conns);
    flags.val = 0;
    flags.syn = 1;
    flags.ack = 1;
    tcp_out(conn, NULL, 0, flags);
    return false;
}

bool tcp_input(struct sk_buff *skb)
{
    struct tcp_hdr *hdr = (struct tcp_hdr *) skb->data;
    struct tcp_sock *sock = tcp_socks_manager_find(
        IF_TCP_SOCKS_MANAGER(skb->intf), NTOHS(hdr->dst_port));
    if (!sock)
        return false;
    struct tcp_connection_sock *conn = find_matching_connection(sock,
        skb->ip_hdr->src_ip, NTOHS(hdr->src_port));
    if (conn)
        return conn->state(conn, skb);
    else if (sock->listening && hdr->flags.syn && !hdr->flags.ack)
        return handle_new_connection(sock, skb);
    return false;
}

bool tcp_out(struct tcp_connection_sock *conn, char *buff, size_t size,
             struct tcp_flags flags)
{
    struct sk_buff *skb = alloc_skb(BUFF_HEADROOM + BUFF_TAILROOM + \
        TCP_HDR_SIZE + size);
    skb->intf = conn->parent->intf;
    skb_reserve(skb, BUFF_HEADROOM);
    struct tcp_hdr *hdr = (struct tcp_hdr *) skb_put(skb, TCP_HDR_SIZE);
    hdr->src_port = HTONS(conn->parent->local_port);
    hdr->dst_port = HTONS(conn->remote_port);
    if (conn->snt.nxt + size > conn->snt.una + conn->snt.wnd)
        return -1;
    hdr->seq = HTONL(conn->snt.nxt);
    hdr->ack = HTONL(conn->rcv.nxt);
    hdr->flags.val = flags.val;
    hdr->wnd_sz = HTONS(conn->rcv.wnd);
    hdr->hlen = 5;
    if (size) {
        memcpy(skb_put(skb, size), buff, size);
        conn->snt.nxt += size;
    }
    if (flags.syn)
        conn->snt.nxt += 1;
    if (flags.fin)
        conn->snt.nxt += 1;
    fill_tcp_cksum(hdr, skb->len, conn->parent->local_ip, conn->remote_ip);
    return ip_output(skb->intf->node, conn->remote_ip, IP_PROTO_TCP, skb);
}

int tcp_socket(struct node *node)
{
    for (size_t i=0; i < MAX_TCP_SOCKS; i++) {
        if (!fd_to_sock[i]) {
            struct tcp_sock *sock = calloc(1, sizeof(struct tcp_sock));
            INIT_LIST_HEAD(&sock->conns);
            INIT_LIST_HEAD(&sock->closed_conns);
            sock->node = node;
            fd_to_sock[i] = sock;
            return i;
        }
    }
    return -1;
}

int tcp_close(int sockfd) {
    if (!is_good_tcp_sock(sockfd))
        return -1;
    struct tcp_sock *sock = fd_to_sock[sockfd];
    fd_to_sock[sockfd] = NULL;
    if (sock->bound) {
        list_del(&sock->list);
    }
    free(sock);
    return 0;
}

int tcp_bind(int sockfd, struct ip_addr ip, u16 port)
{
    if (!is_good_tcp_sock(sockfd) || fd_to_sock[sockfd]->bound)
        return -1;
    struct tcp_sock *sock = fd_to_sock[sockfd];
    struct intf *intf = _get_matching_interface(sock->node, ip);
    if (!intf)
        return -1;
    sock->bound = true;
    sock->intf = intf;
    sock->local_ip.iaddr = ip.iaddr;
    sock->local_port = port;
    list_add(&sock->list, &IF_TCP_SOCKS_MANAGER(intf)->socks);
    return 0;
}

int tcp_listen(int sockfd)
{
    if (!is_good_tcp_sock(sockfd) || !fd_to_sock[sockfd]->bound)
        return -1;
    fd_to_sock[sockfd]->listening = true;
    return 0;
}

int tcp_accept(int sockfd, struct ip_addr *ip, u16 *port)
{
    if (!is_good_tcp_sock(sockfd) || !fd_to_sock[sockfd]->listening)
        return -1;
    struct tcp_sock *sock = fd_to_sock[sockfd];
    struct tcp_connection_sock *conn;
    list_for_each_entry(conn, &sock->conns, list) {
        if (conn->state == handle_estab) {
            for (int i=0; i < MAX_TCP_CONN_SOCKS; i++) {
                if (!fd_conn_to_sock[i]) {
                    fd_conn_to_sock[i] = conn;
                    if (ip)
                        ip->iaddr = conn->remote_ip.iaddr;
                    if (port)
                        *port = conn->remote_port;
                    return i;
                }
            }
            return -1;
        }
    }
    return -1;
}

ssize_t tcp_recv(int connfd, char *buff, size_t size)
{
    if (!is_good_tcp_connection_sock(connfd)
            || fd_conn_to_sock[connfd]->state != handle_estab)
        return -1;
    struct tcp_connection_sock *conn = fd_conn_to_sock[connfd];
    if (!list_empty(&conn->rx_skb)) {
        struct sk_buff *skb = list_last_entry(&conn->rx_skb,
                                              struct sk_buff,
                                              list);
        ssize_t final_size = MIN(skb->len, size);
        memcpy(buff, skb->data, final_size);
        skb_pull(skb, final_size);
        if (!skb->len) {
            list_del(&skb->list);
            free_skb(skb);
        }
        return final_size;
    }
    return -1;
}

ssize_t tcp_send(int connfd, char *buff, size_t size)
{
    if (!is_good_tcp_connection_sock(connfd)
            || fd_conn_to_sock[connfd]->state != handle_estab)
        return -1;
    struct tcp_connection_sock *conn = fd_conn_to_sock[connfd];
    ssize_t final_size = MIN(size, MAX_TCP_PAYLOAD);
    struct tcp_flags flags;
    flags.val = 0;
    flags.ack = 1;
    bool result = tcp_out(conn, buff, final_size, flags);
    if (!result)
        return -1;
    return final_size;
}

void init_tcp_socks_manager(struct tcp_socks_manager *manager)
{
    INIT_LIST_HEAD(&manager->socks);
}

void dump_tcp_socks_manager(struct tcp_socks_manager *manager)
{
    struct tcp_sock *sock;
    printf("TCP Sockets: ");
    list_for_each_entry(sock, &manager->socks, list) {
        printf("%hu ", sock->local_port);
    }
    printf("\n");
}
