#ifndef COMM_H
#define COMM_H
#include <stdbool.h>
#include <stdint.h>
#define RECV_BUFFER_SIZE (2048)
struct intf;
struct graph;
struct node;
struct sk_buff;

struct comm
{
    int sock;
    uint16_t port;
};

bool send_bytes_out(char *bytes, size_t size, struct intf *intf);

bool send_pkt_out(struct sk_buff *skb, struct intf *intf);

bool pkt_receive(struct intf *intf, struct sk_buff *skb);

bool send_pkt_flood(struct node *node, struct intf *exempted_intf,
                    struct sk_buff *skb);

bool send_bytes_flood(struct node *node, struct intf *exempted_intf,
                      char *bytes, size_t size);

void start_recv_thread();

void init_udp_socket(struct comm *comm);

void destroy_udp_socket(struct comm *comm);

typedef void (*data_callback)(char *bytes, size_t size, struct intf *intf);

void set_receive_callback(data_callback cb);

void set_send_callback(data_callback cb);

void stop_recv_thread();

#endif
