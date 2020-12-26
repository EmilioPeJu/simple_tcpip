#ifndef COMM_H
#define COMM_H
#include <stdint.h>

struct intf;
struct graph;
struct node;

struct comm
{
    int sock;
    uint16_t port;
};

int send_pkt_out(char *pkt, size_t pkt_size, struct intf *intf);

int pkt_receive(struct intf *intf, char *pkt, size_t pkt_size);

int send_pkt_flood(struct node *node, struct intf *exempted_intf, char *pkt,
                   size_t pkt_size);

void start_recv_thread();

void init_udp_socket(struct comm *comm);

void destroy_udp_socket(struct comm *comm);

#endif
