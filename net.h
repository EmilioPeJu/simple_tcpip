#ifndef NET_H
#define NET_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "address.h"
#include "arp.h"
#include "switch.h"

struct node;
struct intf;

struct node_nw_prop {
    bool is_lb_addr_config;
    struct ip_addr lb_addr;
    struct arp_table arp_table;
    struct mac_table mac_table;
};

struct  intf_nw_prop {
    struct mac_addr mac_addr;
    bool is_ip_addr_config;
    struct ip_addr ip_addr;
    char mask;
};

bool node_set_loopback_address(struct node *node, char *ip_addr);

bool node_set_intf_ip_addr(struct node *node, char *if_name,
                           char *ip_addr, char mask);

inline void init_intf_nw_prop(struct intf_nw_prop *props)
{
    memset(props, 0, sizeof(*props));
}

inline void init_node_nw_prop(struct node_nw_prop *props)
{
    memset(props, 0, sizeof(*props));
    init_arp_table(&props->arp_table);
    init_mac_table(&props->mac_table);
}

void interface_assign_mac_address(struct intf *intf);

struct intf *_get_matching_subnet_interface(struct node *node,
                                            struct ip_addr *ip);

struct intf *get_matching_subnet_interface(struct node *node, char *ip_addr);

#define IF_MAC(if_ptr) (&(if_ptr)->intf_nw_prop.mac_addr)
#define IF_IP(if_ptr) (&(if_ptr)->intf_nw_prop.ip_addr)
#define NODE_LO_ADDR(node_ptr) (&(node_ptr)->node_nw_prop.lb_addr)
#define NODE_ARP_TABLE(node_ptr) (&(node_ptr)->node_nw_prop.arp_table)
#define IS_INTF_L3_MODE(if_ptr) ((if_ptr)->intf_nw_prop.is_ip_addr_config)
#endif
