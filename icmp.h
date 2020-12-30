#ifndef ICMP_H
#define ICMP_H
#include <stdbool.h>
#include <stddef.h>
#include "address.h"
#define ICMP_ECHO_REQUEST_TYPE (8)
#define ICMP_ECHO_REQUEST_CODE (0)
#define ICMP_ECHO_REPLY_TYPE (0)
#define ICMP_ECHO_REPLY_CODE (0)
struct sk_buff;
struct node;

struct icmp_hdr {
    u8 type;
    u8 code;
    u16 cksum;
    u8 rest[4];
};

bool icmp_input(struct sk_buff *skb);

bool icmp_out(struct node *node, struct ip_addr ip,
              u8 type, u8 code, char *payload, size_t size);

u16 calc_icmp_cksum(struct icmp_hdr *hdr);

void fill_icmp_cksum(struct icmp_hdr *hdr);

bool check_icmp_cksum(struct icmp_hdr *hdr);

bool ping(struct node *node, char *ip_addr);

#define ICMP_HDR_SIZE (sizeof(struct icmp_hdr))

#endif
