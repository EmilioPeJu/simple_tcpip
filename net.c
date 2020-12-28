#include <stdint.h>
#include <stdio.h>
#include "net.h"
#include "graph.h"
#include "utils.h"

static inline bool get_raw_ip_addr(const char *ip_addr, u8 *raw_ip_addr)
{
    uint32_t ip_num;
    if (!(ip_num=convert_ip_from_str_to_int(ip_addr))) {
        return false;
    }
    *((uint32_t *) raw_ip_addr) = ip_num;
    return true;
}

bool node_set_loopback_address(struct node *node, char *ip_addr)
{
    if (!get_raw_ip_addr(ip_addr, NODE_LO_ADDR(node)->addr))
        return false;
    node->node_nw_prop.is_lb_addr_config = true;
    return true;
}

bool node_set_intf_ip_addr(struct node *node, char *if_name, char *ip_addr,
                           char mask)
{
    struct intf *intf = get_node_if_by_name(node, if_name);
    if (!intf)
        return false;
    if(!get_raw_ip_addr(ip_addr, IF_IP(intf)->addr))
        return false;
    intf->intf_nw_prop.mask = mask;
    intf->intf_nw_prop.is_ip_addr_config = true;
    return true;
}

static inline uint64_t hashcode(const char *string)
{
    uint64_t result = 7;
    for (size_t i=0; i < strlen(string); i++)
        result *= string[i] * 31;
    return result;
}

void interface_assign_mac_address(struct intf *intf)
{
    uint64_t code = hashcode(intf->node->name) * hashcode(intf->name);
    memset(IF_MAC(intf), 0, MAC_ADDR_SIZE);
    memcpy(IF_MAC(intf), &code, MAC_ADDR_SIZE);
}

struct intf *_get_matching_subnet_interface(struct node *node,
                                            struct ip_addr *ip)
{
    struct intf *intf;
    for (size_t i=0; i < MAX_INTFS_PER_NODE; i++) {
        intf = node->intfs[i];
        if (intf && IS_INTF_L3_MODE(intf)) {
            uint32_t mask = intf->intf_nw_prop.mask;
            uint32_t ip1 = ip->iaddr & ((1<<mask) - 1);
            uint32_t ip2 = IF_IP(intf)->iaddr & ((1<<mask) - 1);
            if (ip1 == ip2)
                return intf;
        }
    }
    return NULL;
}

struct intf *get_matching_subnet_interface(struct node *node, char *ip_addr)
{
    struct ip_addr ip;
    ip.iaddr = convert_ip_from_str_to_int(ip_addr);
    return _get_matching_subnet_interface(node, &ip);
}

