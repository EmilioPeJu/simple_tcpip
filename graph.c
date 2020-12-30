#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "arp.h"
#include "graph.h"
#include "ip.h"

struct graph *create_new_graph(char *name)
{
    struct graph *graph = calloc(1, sizeof(struct graph));
    strncpy(graph->name, name, MAX_TOPO_NAME_SIZE);
    INIT_LIST_HEAD(&graph->nodes);
    return graph;
}

void destroy_graph(struct graph *graph)
{
    struct node *node, *tmp;
    list_for_each_entry_safe(node, tmp, &graph->nodes, list) {
        destroy_node(node);
    }
    free(graph);
}

struct node *create_graph_node(struct graph *graph, const char *node_name)
{
    struct node *node = calloc(1, sizeof(struct node));
    strncpy(node->name, node_name, MAX_NODE_NAME_SIZE);
    init_node_nw_prop(&node->node_nw_prop);
    list_add(&node->list, &graph->nodes);
    return node;
}

void destroy_node(struct node *node)
{
    list_del(&node->list);
    for (size_t i=0; i < MAX_INTFS_PER_NODE; i++) {
        if (node->intfs[i]) {
            destroy_link(node->intfs[i]->link);
        }
    }
    destroy_node_nw_prop(&node->node_nw_prop);
    free(node);
}

void insert_link_between_two_nodes(struct node *node1,
                                   struct node *node2,
                                   const char *from_ifname,
                                   const char *to_ifname,
                                   uint32_t cost)
{
    struct link *link = calloc(1, sizeof(struct link));
    link->cost = cost;
    init_interface(&link->intf1, from_ifname, node1, link);
    init_interface(&link->intf2, to_ifname, node2, link);
}

void destroy_interface(struct intf *intf)
{
    destroy_udp_socket(&intf->comm);
}

void destroy_link(struct link *link)
{
    struct node *node1 = link->intf1.node;
    struct node *node2 = link->intf2.node;
    for (size_t i=0; i < MAX_INTFS_PER_NODE; i++) {
        if (node1->intfs[i] &&
                node1->intfs[i]->link == link) {
            node1->intfs[i] = NULL;
        }
        if (node2->intfs[i] &&
                node2->intfs[i]->link == link) {
            node2->intfs[i] = NULL;
        }
    }
    destroy_interface(&link->intf1);
    destroy_interface(&link->intf2);
    free(link);
}

static void dump_intf_nw_prop(struct intf_nw_prop *prop)
{
    u8 *mac = prop->mac_addr.addr;
    u8 *ip = prop->ip_addr.addr;
    printf("\tmac: %02x:%02x:%02x:%02x:%02x:%02x, ip: ", mac[0], mac[1],
           mac[2], mac[3], mac[4], mac[5]);
    if (!prop->is_ip_addr_config)
        printf("Not configured");
    else
        printf("%u.%u.%u.%u/%u", ip[0], ip[1], ip[2], ip[3], prop->mask);
}

static void dump_interface(struct intf *intf)
{
    printf("\tInterface: %s, nbr: %s, cost: %u, ",
           intf->name,
           get_nbr_node(intf)->name,
           intf->link->cost);
    dump_intf_nw_prop(&intf->intf_nw_prop);
    printf("\n");
}

void dump_nw_graph(struct graph *graph)
{
    printf("Graph name: %s\n", graph->name);
    struct node *node;
    list_for_each_entry(node, &graph->nodes, list) {
        printf("Node name: %s", node->name);
        if (node->node_nw_prop.is_lb_addr_config) {
            u8 *ip = node->node_nw_prop.lb_addr.addr;
            printf(", loopback ip: %u.%u.%u.%u",
                   ip[0], ip[1], ip[2], ip[3]);
        }
        printf("\n");
        for (size_t i=0; i < MAX_INTFS_PER_NODE; i++) {
            if (node->intfs[i]) {
                dump_interface(node->intfs[i]);
            }
        }
        dump_arp_table(NODE_ARP_TABLE(node));
        dump_rt_table(NODE_RT_TABLE(node));
    }
}

void init_interface(struct intf *intf, const char *name,  struct node *node,
                    struct link *link)
{
    strncpy(intf->name, name, MAX_IF_NAME_SIZE);
    intf->node = node;
    intf->link = link;
    size_t slot = get_node_intf_available_slot(node);
    assert(slot != -1);
    node->intfs[slot] = intf;
    init_intf_nw_prop(&intf->intf_nw_prop);
    interface_assign_mac_address(intf);
    init_udp_socket(&intf->comm);
}
