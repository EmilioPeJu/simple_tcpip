#ifndef FIXTURE_H
#define FIXTURE_H
#define TEST_ETH01_IP_STR "192.168.89.1"
#define TEST_ETH10_IP_STR "192.168.89.2"

struct graph *create_test_topology();

void destroy_test_topology();

void wait_test_graph_received(size_t n);

char *get_test_recv_data(size_t n);

size_t get_test_recv_len(size_t n);

struct intf *get_test_recv_intf(size_t n);

size_t get_test_recv_n();

void reset_test_recv();

#endif
