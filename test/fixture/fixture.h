#ifndef FIXTURE_H
#define FIXTURE_H

struct graph *create_test_topology();

void destroy_test_topology();

void wait_has_received(size_t n);

char *get_test_recv_data(size_t n);

size_t get_test_recv_len(size_t n);

struct intf *get_test_recv_intf(size_t n);

size_t get_test_recv_n();

void reset_test_recv();

#endif
