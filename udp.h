#ifndef UDP_H
#define UDP_H
#include <stdbool.h>
#include "address.h"
#include "list.h"
#define MAX_UDP_SOCKS (8)
#define MAX_UDP_PAYLOAD (1446)
#define UDP_HDR_SIZE (sizeof(struct udp_hdr))

struct node;
struct sk_buff;

struct udp_hdr {
    u16 src_port;
    u16 dst_port;
    u16 total_len;
    u16 cksum; // filling not mandatory
};

struct udp_sock {
    bool bound;
    struct node *node;
    // struct ip_addr local_ip;
    struct intf *intf;
    u16 local_port;
    struct list_head rx_skb;
    struct list_head list;
};

struct udp_socks_manager {
    struct list_head socks;
};

void init_udp_socks_manager(struct udp_socks_manager *manager);

struct udp_sock *udp_socks_manager_find(struct udp_socks_manager *manager,
                                       u16 port);

bool udp_input(struct sk_buff *skb);

bool udp_out(struct udp_sock *sock, char *buff, size_t size, struct ip_addr ip,
             u16 port);

int udp_socket(struct node *node);

int udp_bind(int sockfd, struct ip_addr ip, u16 port);

ssize_t udp_recvfrom(int sockfd, char *buff, size_t size, struct ip_addr *ip,
                     u16 *port);

ssize_t udp_sendto(int sockfd, char *buff, size_t size, struct ip_addr ip,
                   u16 port);

int udp_close(int sockfd);

void dump_udp_socks_manager(struct udp_socks_manager *manager);

#endif
