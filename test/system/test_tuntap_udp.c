#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "graph.h"
#include "comm.h"
#include "fixture.h"
#include "udp.h"
#include "utils.h"

int main()
{
    struct graph *tgraph = create_new_graph("graph");
    struct node *tnode0 = create_graph_node(tgraph, "node0");
    insert_tuntap_interface(tnode0, "ethX");
    struct intf *ethX = get_node_if_by_name(tnode0, "ethX");
    node_set_intf_ip_addr(tnode0, "ethX", "10.10.0.2", 24);
    int sock;
    if ((sock = udp_socket(tnode0)) < 0) {
        printf("socket: error\n");
        return 1;
    }
    if(udp_bind(sock, *IF_IP(ethX), 2000)) {
        printf("bind: error\n");
        return 1;
    }
    start_recv_thread();
    char buffer[256];
    char msg[256];
    size_t ntimes = 0;
    while (true) {
        ssize_t nrecv;
        struct ip_addr ip;
        u16 port;
        while ((nrecv=udp_recvfrom(sock, buffer, 256, &ip, &port)) > 0) {
            printf("UDP received %ld bytes from %u.%u.%u.%u:%hu %s\n", nrecv,
                    ip.addr[0], ip.addr[1], ip.addr[2], ip.addr[3], port,
                    buffer);
            snprintf(msg, 256, "Recv number %lu\n", ntimes++);
            udp_sendto(sock, msg, strlen(msg), ip, port);
        }
        sleep(1);
    }
    dump_nw_graph(tgraph);
    stop_recv_thread();
    destroy_graph(tgraph);
    return 0;
}
