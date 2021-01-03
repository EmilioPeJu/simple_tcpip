#ifndef TCP_H
#define TCP_H
#include <stdbool.h>
#include "address.h"
#include "list.h"
#define MAX_TCP_SOCKS (8)
#define MAX_TCP_CONN_SOCKS (8)
#define MAX_TCP_PAYLOAD (1446)
#define TCP_HDR_SIZE (sizeof(struct tcp_hdr))
#define TCP_WINDOW_SIZE (1500)
#define PSEUDO_IP_HDR_SIZE (sizeof(struct pseudo_ip_hdr))
struct tcp_connection_sock;
struct sk_buff;
typedef bool (*tcp_handle)(struct tcp_connection_sock *conn, struct sk_buff *skb);

struct node;
struct sk_buff;

struct pseudo_ip_hdr {
    struct ip_addr src_ip;
    struct ip_addr dst_ip;
    u8 rsrv;
    u8 proto;
    u16 len;
} __attribute__((packed));

struct tcp_flags {
    union {
        u8 val;
        struct {
            u8 fin:1;
            u8 syn:1;
            u8 rst:1;
            u8 push:1;
            u8 ack:1;
            u8 urg:1;
            u8 rsrv2:1;
            u8 rsrv3:1;
        };
    };
};

struct tcp_hdr {
    u16 src_port;
    u16 dst_port;
    u32 seq;
    u32 ack;
    // little endian is assumed, which means hlen is lower in memory
    u8 rsrv1:4;
    u8 hlen:4;
    struct tcp_flags flags;
    u16 wnd_sz;
    u16 cksum;
    u16 urgp;
} __attribute__((packed));

struct tcp_sock {
    bool bound;
    bool listening;
    struct node *node;
    struct ip_addr local_ip;
    struct intf *intf;
    u16 local_port;
    struct list_head conns;
    struct list_head closed_conns;
    struct list_head list;
};

struct tcp_connection_sock {
    struct tcp_sock *parent;
    tcp_handle state;
    struct ip_addr remote_ip;
    u16 remote_port;
    struct {
        u32 una;
        u32 nxt;
        u16 wnd;
        u32 iss;
        //u16 up;
        //u32 wl1;
        //u32 wl2;
    } snt;
    struct {
        u32 nxt;
        u16 wnd;
        u32 irs;
        //u16 up;
    } rcv;
    struct list_head rx_skb;
    struct list_head tx_skb;
    struct list_head list;
};

struct tcp_socks_manager {
    struct list_head socks;
};

void init_tcp_socks_manager(struct tcp_socks_manager *manager);

bool tcp_input(struct sk_buff *skb);

bool tcp_out(struct tcp_connection_sock *conn, char *buff, size_t size,
             struct tcp_flags flags);

bool tcp_syn_ack(struct tcp_connection_sock *conn);

int tcp_socket(struct node *node);

int tcp_bind(int sockfd, struct ip_addr ip, u16 port);

int tcp_listen(int sockfd);

int tcp_accept(int sockfd, struct ip_addr *ip, u16 *port);

ssize_t tcp_recv(int con_sockfd, char *buff, size_t size);

ssize_t tcp_send(int con_sockfd, char *buff, size_t size);

int tcp_connection_close(int sockfd);

int tcp_close(int sockfd);

void dump_tcp_socks_manager(struct tcp_socks_manager *manager);

u16 calc_tcp_cksum(struct tcp_hdr *hdr, size_t size,
                   struct ip_addr src_ip, struct ip_addr dst_ip);

#endif
