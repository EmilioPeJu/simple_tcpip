#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "arp.h"
#include "graph.h"
#include "comm.h"
#include "fixture.h"
#include "udp.h"
#include "utils.h"

int main()
{
    char buffer[32];
    struct graph *graph = create_test_topology();
    struct node *node0 = get_node_by_node_name(graph, "node0");
    struct node *node1 = get_node_by_node_name(graph, "node1");
    struct intf *eth01 = get_node_if_by_name(node0, "eth01");
    struct intf *eth10 = get_node_if_by_name(node1, "eth10");
    start_recv_thread();
    int sock0 = udp_create_socket(node0);
    int sock1 = udp_create_socket(node1);
    if (udp_bind(sock0, *IF_IP(eth01), 1000)) {
        printf("Problem binding\n");
        return 1;
    }
    if (udp_bind(sock1, *IF_IP(eth10), 2000)) {
        printf("Problem binding\n");
        return 1;
    }
    ssize_t len;
    char *msg = "hello world\n";
    len = udp_sendto(sock0, msg, strlen(msg), *IF_IP(eth10), 2000);
    printf("UDP sent %ld bytes: %s\n", len, msg);
    wait_test_graph_received(3);
    len = udp_recvfrom(sock1, buffer, 5, NULL, NULL);
    printf("UDP received %ld bytes: %s\n", len, buffer);
    len = udp_recvfrom(sock1, buffer, 64, NULL, NULL);
    printf("UDP received %ld bytes: %s\n", len, buffer);
    dump_nw_graph(graph);
    udp_close(sock0);
    udp_close(sock1);
    stop_recv_thread();
    destroy_test_topology();
    return 0;
}
