#ifndef NET_H
#define NET_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "address.h"
#include "arp.h"
#include "ip.h"
#include "switch.h"
#include "tcp.h"
#include "udp.h"
#define BUFF_HEADROOM (60)
#define BUFF_TAILROOM (46)

struct node;
struct intf;

struct node_nw_prop {
    bool is_lb_addr_config;
    struct ip_addr lb_addr;
    struct arp_table arp_table;
    struct mac_table mac_table;
    struct rt_table rt_table;
};

struct  intf_nw_prop {
    struct tcp_socks_manager tcp_socks_manager;
    struct udp_socks_manager udp_socks_manager;
    struct mac_addr mac_addr;
    bool is_ip_addr_config;
    struct ip_addr ip_addr;
    char mask;
};

bool node_set_loopback_address(struct node *node, char *ip_addr);

bool node_set_intf_ip_addr(struct node *node, char *if_name,
                           char *ip_addr, char mask);

void init_intf_nw_prop(struct intf_nw_prop *props);

void init_node_nw_prop(struct node_nw_prop *props);

void destroy_node_nw_prop(struct node_nw_prop *props);

void interface_assign_mac_address(struct intf *intf);

struct intf *_get_matching_subnet_interface(struct node *node,
                                            struct ip_addr ip);

struct intf *get_matching_subnet_interface(struct node *node, char *ip_addr);

struct intf *_get_matching_interface(struct node *node, struct ip_addr ip);

#define IF_MAC(if_ptr) (&(if_ptr)->intf_nw_prop.mac_addr)
#define IF_IP(if_ptr) (&(if_ptr)->intf_nw_prop.ip_addr)
#define IF_UDP_SOCKS_MANAGER(if_ptr) \
    (&(if_ptr)->intf_nw_prop.udp_socks_manager)
#define IF_TCP_SOCKS_MANAGER(if_ptr) \
    (&(if_ptr)->intf_nw_prop.tcp_socks_manager)
#define NODE_LO_ADDR(node_ptr) (&(node_ptr)->node_nw_prop.lb_addr)
#define NODE_ARP_TABLE(node_ptr) (&(node_ptr)->node_nw_prop.arp_table)
#define NODE_RT_TABLE(node_ptr) (&(node_ptr)->node_nw_prop.rt_table)
#define IS_INTF_L3_MODE(if_ptr) ((if_ptr)->intf_nw_prop.is_ip_addr_config)
#endif
