#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "graph.h"
#include "comm.h"
#include "fixture.h"
#include "tcp.h"
#include "utils.h"

int main()
{
    struct graph *tgraph = create_new_graph("graph");
    struct node *tnode0 = create_graph_node(tgraph, "node0");
    insert_tuntap_interface(tnode0, "ethX");
    struct intf *ethX = get_node_if_by_name(tnode0, "ethX");
    node_set_intf_ip_addr(tnode0, "ethX", "10.10.0.2", 24);
    int sock;
    if ((sock = tcp_socket(tnode0)) < 0) {
        printf("socket: error\n");
        return 1;
    }
    if (tcp_bind(sock, *IF_IP(ethX), 2000)) {
        printf("bind: error\n");
        return 1;
    }
    if (tcp_listen(sock)){
        printf("listen: error\n");
        return 1;
    }
    start_recv_thread();
    char buffer[256];
    char msg[256];
    size_t ntimes = 0;
    int conn;
    struct ip_addr ip;
    u16 port;
    while ((conn=tcp_accept(sock, &ip, &port)) < 0) {
        sleep(1);
    }
    printf("Connection from %u.%u.%u.%u:%hu\n",
            ip.addr[0], ip.addr[1], ip.addr[2], ip.addr[3], port);
    ssize_t nrecv;
    while(true) {
        while ((nrecv=tcp_recv(sock, buffer, 256)) > 0) {
            printf("TCP received %ld bytes: %s\n", nrecv, buffer);
            snprintf(msg, 256, "Recv number %lu\n", ntimes++);
            tcp_send(sock, msg, strlen(msg));
            sleep(1);
        }
    }
    dump_nw_graph(tgraph);
    stop_recv_thread();
    destroy_graph(tgraph);
    return 0;
}
