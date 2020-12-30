#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "arp.h"
#include "graph.h"
#include "comm.h"
#include "fixture.h"
#include "icmp.h"
#include "utils.h"

int main()
{
    struct graph *graph = create_test_topology();
    dump_nw_graph(graph);
    struct node *node0 = get_node_by_node_name(graph, "node0");
    start_recv_thread();
    if(!ping(node0, TEST_ETH10_IP_STR)){
        printf("Ping msg dropped\n");
    }
    wait_test_graph_received(2);
    dump_nw_graph(graph);
    stop_recv_thread();
    destroy_test_topology();
    return 0;
}
