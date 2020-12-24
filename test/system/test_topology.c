#include "graph.h"

int main()
{
    struct graph *graph = create_new_graph("Hello graph");
    struct node *node1 = create_graph_node(graph, "node1");
    struct node *node2 = create_graph_node(graph, "node2");
    struct node *node3 = create_graph_node(graph, "node3");
    insert_link_between_two_nodes(node1, node2, "eth0", "eth10", 1);
    insert_link_between_two_nodes(node1, node3, "eth1", "eth20", 1);
    insert_link_between_two_nodes(node2, node3, "eth11", "eth21", 1);
    dump_graph(graph);
    destroy_node(node1);
    destroy_node(node2);
    destroy_node(node3);
    destroy_graph(graph);
    return 0;
}
