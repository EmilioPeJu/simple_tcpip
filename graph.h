#ifndef GRAPH_H
#define GRAPH_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "comm.h"
#include "list.h"
#include "net.h"
#define MAX_IF_NAME_SIZE (15)
#define MAX_TOPO_NAME_SIZE (32)
#define MAX_NODE_NAME_SIZE (32)
#define MAX_INTFS_PER_NODE (4)
struct link;
struct node;

#define COMM_INTF(comm_ptr) container_of(comm_ptr, struct intf, comm)

struct intf {
    char name[MAX_IF_NAME_SIZE + 1];
    struct node *node;
    struct link *link;
    struct intf_nw_prop intf_nw_prop;
    struct comm comm;
};

struct link {
    struct intf intf1;
    struct intf intf2;
    uint32_t cost;
};

struct node {
    char name[MAX_NODE_NAME_SIZE + 1];
    struct intf *intfs[MAX_INTFS_PER_NODE];
    struct list_head list;
    struct node_nw_prop node_nw_prop;
};

struct graph {
    char name[MAX_TOPO_NAME_SIZE + 1];
    struct list_head nodes;
};

struct graph *create_new_graph(char *name);

void destroy_graph(struct graph *graph);

struct node *create_graph_node(struct graph *graph, const char *node_name);

void destroy_node(struct node *node);

void insert_link_between_two_nodes(struct node *node1,
                                   struct node *node2,
                                   const char *from_ifname,
                                   const char *to_ifname,
                                   uint32_t cost);

void insert_tuntap_interface(struct node *node, const char *name);

void destroy_link(struct link *link);

void destroy_interface(struct intf *intf);

void dump_nw_graph(struct graph *graph);

static inline struct intf *get_nbr_if(struct intf *intf) {
    if (!intf->link)
        return NULL;
    return intf == &intf->link->intf1 ? &intf->link->intf2 : &intf->link->intf1;
}

static inline struct node *get_nbr_node(struct intf *intf) {
    struct intf *nbr_intf = get_nbr_if(intf);
    if (nbr_intf)
        return nbr_intf->node;
    return NULL;
}

static inline int get_node_intf_available_slot(struct node *node)
{
    for (int i=0; i < MAX_INTFS_PER_NODE; i++) {
        if (!node->intfs[i])
            return i;
    }
    return -1;
}

static inline struct intf *get_node_if_by_name(struct node *node,
                                                const char *if_name)
{
    for (size_t i=0; i < MAX_INTFS_PER_NODE; i++) {
        if (node->intfs[i] &&
                strncmp(node->intfs[i]->name, if_name, MAX_IF_NAME_SIZE) == 0)
            return node->intfs[i];
    }
    return NULL;
}

static inline struct node *get_node_by_node_name(struct graph *graph,
                                                 const char *node_name)
{
    struct node *node;
    list_for_each_entry(node, &graph->nodes, list) {
        if (strncmp(node->name, node_name, MAX_NODE_NAME_SIZE) == 0)
            return node;
    }
    return NULL;
}

void init_interface(struct intf *intf, const char *name, struct node *node,
                    struct link *link, enum comm_type comm_type);

#endif
