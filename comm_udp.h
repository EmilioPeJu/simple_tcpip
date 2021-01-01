#ifndef COMM_UDP_H
#define COMM_UDP_H
#include <stddef.h>

struct comm;

extern struct comm_ops udp_comm_ops;

bool udp_init_comm(struct comm *comm);

bool udp_destroy_comm(struct comm *comm);

ssize_t udp_send_bytes(struct comm *comm, char *bytes, size_t size);

ssize_t udp_recv_bytes(struct comm *comm, char *bytes, size_t size);

#endif
