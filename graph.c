#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph.h"

struct graph *create_new_graph(char *name)
{
    struct graph *graph = calloc(1, sizeof(struct graph));
    strncpy(graph->name, name, MAX_TOPO_NAME_SIZE);
    INIT_LIST_HEAD(&graph->nodes);
    return graph;
}

void destroy_graph(struct graph *graph)
{
    free(graph);
}

struct node *create_graph_node(struct graph *graph, const char *node_name)
{
    struct node *node = calloc(1, sizeof(struct node));
    strncpy(node->name, node_name, MAX_NODE_NAME_SIZE);
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
    strncpy(link->intf1.name, from_ifname, MAX_IF_NAME_SIZE);
    strncpy(link->intf2.name, to_ifname, MAX_IF_NAME_SIZE);
    link->intf1.link = link;
    link->intf2.link = link;
    link->intf1.node = node1;
    link->intf2.node = node2;
    size_t slot1 = get_node_intf_available_slot(node1);
    size_t slot2 = get_node_intf_available_slot(node2);
    node1->intfs[slot1] = &link->intf1;
    node2->intfs[slot2] = &link->intf2;
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
    free(link);
}

void dump_graph(struct graph *graph)
{
    printf("Graph name: %s\n", graph->name);
    struct node *node;
    list_for_each_entry(node, &graph->nodes, list) {
        printf("Node name: %s\n", node->name);
        for (size_t i=0; i < MAX_INTFS_PER_NODE; i++) {
            if (node->intfs[i]) {
                printf("\tInterface: %s, nbr: %s, cost: %u\n",
                       node->intfs[i]->name,
                       get_nbr_node(node->intfs[i])->name,
                       node->intfs[i]->link->cost);
            }
        }
    }
}
