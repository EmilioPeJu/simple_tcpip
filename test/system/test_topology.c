#include <string.h>
#include <unistd.h>
#include "arp.h"
#include "graph.h"
#include "comm.h"
#include "fixture.h"
#include "utils.h"

int main()
{
    struct graph *graph = create_test_topology();
    dump_nw_graph(graph);
    struct node *node0 = get_node_by_node_name(graph, "node0");
    struct ip_addr ip;
    start_recv_thread();
    char *msg = "Hello";
    send_bytes_out(msg, strlen(msg), get_node_if_by_name(node0, "eth01"));
    ip.iaddr = convert_ip_from_str_to_int("192.168.89.2");
    send_arp_broadcast_request(get_node_if_by_name(node0, "eth01"), ip);
    wait_has_received(2);
    dump_arp_table(NODE_ARP_TABLE(node0));
    stop_recv_thread();
    destroy_test_topology();
    return 0;
}
