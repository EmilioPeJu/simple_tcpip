#ifndef COMM_TAP_H
#define COMM_TAP_H
#include <stdbool.h>
#include <stddef.h>

struct comm;

extern struct comm_ops tuntap_comm_ops;

bool tuntap_init_comm(struct comm *comm);

bool tuntap_destroy_comm(struct comm *comm);

ssize_t tuntap_send_bytes(struct comm *comm, char *bytes, size_t size);

ssize_t tuntap_recv_bytes(struct comm *comm, char *bytes, size_t size);

#endif
