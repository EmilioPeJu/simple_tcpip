#ifndef NET_H
#define NET_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#define MAC_ADDR_SIZE (6)
#define IP_ADDR_SIZE (4)

struct node;
struct intf;

struct ip_addr {
    char addr[IP_ADDR_SIZE];
};

struct mac_addr {
    char addr[MAC_ADDR_SIZE];
};

struct node_nw_prop {
    bool is_lb_addr_config;
    struct ip_addr lb_addr;
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
}

void interface_assign_mac_address(struct intf *intf);

struct intf *_get_matching_subnet_interface(struct node *node, char *ip_addr);

struct intf *get_matching_subnet_interface(struct node *node, char *ip_addr);

#define IF_MAC(if_ptr) ((if_ptr)->intf_nw_prop.mac_addr.addr)
#define IF_IP(if_ptr) ((if_ptr)->intf_nw_prop.ip_addr.addr)
#define NODE_LO_ADDR(node_ptr) ((node_ptr)->node_nw_prop.lb_addr.addr)
#define IS_INTF_L3_MODE(if_ptr) ((if_ptr)->intf_nw_prop.is_ip_addr_config)
#endif
