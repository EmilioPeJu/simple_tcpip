#ifndef COMM_H
#define COMM_H
#include "address.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#define MAX_COMMS (1024)
#define RECV_BUFFER_SIZE (2048)
#define RECV_RESERVED_HEAD_ROOM (4)
struct comm;
struct intf;
struct graph;
struct node;
struct sk_buff;

enum comm_type {
    CT_NONE,
    CT_UDP,
    CT_TUNTAP
};

struct comm_ops {
    bool (*init_comm)(struct comm *comm);
    bool (*destroy_comm)(struct comm *comm);
    ssize_t (*send_bytes)(struct comm *comm, char *bytes, size_t size);
    ssize_t (*recv_bytes)(struct comm *comm, char *bytes, size_t size);
};

struct comm
{
    enum comm_type comm_type;
    int fd;
    u16 port;
    struct comm_ops *ops;
};

bool init_comm(struct comm *comm, enum comm_type comm_type);

bool destroy_comm(struct comm *comm);

bool set_fd_comm(int fd, struct comm *comm);

struct comm *fd_to_comm(int fd);

ssize_t send_bytes_out(char *bytes, size_t size, struct intf *intf);

bool send_pkt_out(struct intf *intf, struct sk_buff *skb);

bool pkt_receive(struct sk_buff *skb);

bool send_pkt_flood(struct node *node, struct intf *exempted_intf,
                    struct sk_buff *skb);

bool send_bytes_flood(struct node *node, struct intf *exempted_intf,
                      char *bytes, size_t size);

void start_recv_thread();

typedef void (*data_callback)(char *bytes, size_t size, struct intf *intf);

void set_receive_callback(data_callback cb);

void set_send_callback(data_callback cb);

void stop_recv_thread();

#endif
