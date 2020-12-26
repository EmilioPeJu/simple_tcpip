#include <string.h>
#include <unistd.h>
#include "graph.h"
#include "comm.h"

int main()
{
    struct graph *graph = create_new_graph("graph");
    struct node *node0 = create_graph_node(graph, "node0");
    node_set_loopback_address(node0, "122.1.1.0");
    struct node *node1 = create_graph_node(graph, "node1");
    struct node *node2 = create_graph_node(graph, "node2");
    insert_link_between_two_nodes(node0, node1, "eth01", "eth10", 1);
    node_set_intf_ip_addr(node0, "eth01", "20.1.1.1", 24);
    node_set_intf_ip_addr(node1, "eth10", "20.1.1.2", 24);
    insert_link_between_two_nodes(node0, node2, "eth02", "eth20", 1);
    node_set_intf_ip_addr(node0, "eth02", "40.1.1.1", 24);
    node_set_intf_ip_addr(node2, "eth20", "40.1.1.2", 24);
    insert_link_between_two_nodes(node1, node2, "eth12", "eth21", 1);
    node_set_intf_ip_addr(node1, "eth12", "30.1.1.1", 24);
    node_set_intf_ip_addr(node2, "eth21", "30.1.1.2", 24);
    dump_nw_graph(graph);
    start_recv_thread();
    char *msg = "Hello";
    send_pkt_out(msg, strlen(msg), get_node_if_by_name(node0, "eth01"));
    sleep(1);
    destroy_node(node0);
    destroy_node(node1);
    destroy_node(node2);
    destroy_graph(graph);
    return 0;
}
