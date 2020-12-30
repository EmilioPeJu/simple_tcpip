#ifndef IP_H
#define IP_H
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include "address.h"
#include "list.h"
#define IP_PROTO_ICMP (1)

struct intf;
struct node;
struct sk_buff;

struct ip_hdr {
    // assuming little endian
    u8 hlen:4;
    u8 ver:4;
    u8 tos;
    u16 total_len;
    u16 id;
    u16 frag_off;
    u8 ttl;
    u8 proto;
    u16 hcksum;
    struct ip_addr src_ip;
    struct ip_addr dst_ip;
};

u16 calc_ip_hcksum(struct ip_hdr *hdr);

void fill_ip_hcksum(struct ip_hdr *hdr);

bool check_ip_hcksum(struct ip_hdr *hdr);

struct rt_table {
    struct list_head routes;
};

struct rt_entry {
    struct ip_addr dst;
    char mask;
    struct ip_addr next;
    struct intf *intf;
    struct list_head list;
};

// ip routing functions
bool ip_preroute(struct sk_buff *skb);

bool ip_forward(struct sk_buff *skb);

bool ip_input(struct sk_buff *skb);

bool ip_postroute(struct node *node, struct sk_buff *skb);

bool ip_output(struct node *node, struct ip_addr ip, u8 proto,
               struct sk_buff *skb);

// routing table related functions
void init_rt_table(struct rt_table *table);

void destroy_rt_table(struct rt_table *table);

void dump_rt_table(struct rt_table *table);

void rt_table_add_route(struct rt_table *table, struct ip_addr dst,
                        char mask, struct ip_addr next, struct intf *intf);

struct rt_entry *rt_lookup_lpm(struct rt_table *table, struct ip_addr ip);

// auxiliary functions
void set_ip_forward(bool val);

bool is_ip_local(struct node *node, struct ip_addr ip);

void init_ip_hdr(struct ip_hdr *hdr);

#define MASK_IP(ip, mask) ((ip)->iaddr &= ((1<<(mask)) - 1))
#define INT_IP_MASKED(ip, mask) ((ip)->iaddr & ((1<<(mask)) - 1))
#define IP_EQ(ip1, ip2) ((ip1)->iaddr == (ip2)->iaddr)
#define IP_NET_EQ(ip1, ip2, mask) (INT_IP_MASKED(ip1, mask) == INT_IP_MASKED(ip2, mask))
#define RT_TABLE_FOREACH(pos, table) \
  list_for_each_entry(pos, &table->routes, list)
#define IP_HDR_SIZE (sizeof(struct ip_hdr))

#endif
