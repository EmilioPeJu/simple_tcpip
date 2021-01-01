#include <string.h>
#include <unistd.h>
#include "comm.h"
#include "fixture.h"
#include "graph.h"
static struct graph *tgraph;
#define MAX_SAVED_FRAMES (32)
static char recv_data[MAX_SAVED_FRAMES][RECV_BUFFER_SIZE];
static size_t recv_len[MAX_SAVED_FRAMES];
static struct intf *recv_intf[MAX_SAVED_FRAMES];
static size_t  recv_n;

char *get_test_recv_data(size_t n)
{

    if (n < recv_n)
        return recv_data[n];
    return NULL;
}

size_t get_test_recv_len(size_t n)
{
    if (n < recv_n)
        return recv_len[n];
    return 0;
}

struct intf *get_test_recv_intf(size_t n)
{
    if (n < recv_n)
        return recv_intf[n];
    return NULL;
}

size_t get_test_recv_n()
{
    return recv_n;
}

void reset_test_recv()
{
    memset(recv_len, 0, MAX_SAVED_FRAMES*sizeof(size_t));
    memset(recv_intf, 0, MAX_SAVED_FRAMES*sizeof(struct intf *));
    memset(recv_data, 0, MAX_SAVED_FRAMES*RECV_BUFFER_SIZE);
    recv_n = 0;
}

static void receive_callback(char *bytes, size_t size, struct intf *intf)
{
    // memcpy to not get affected by the deallocation
    // of the sk_buff
    memcpy(recv_data[recv_n], bytes, size);
    recv_len[recv_n] = size;
    recv_intf[recv_n] = intf;
    recv_n = (recv_n + 1) % MAX_SAVED_FRAMES;
}

void wait_test_graph_received(size_t n)
{
    // TODO: use condition variable instead
    while (recv_n < n)
        sleep(1);
}

struct graph *create_test_topology()
{
    // set up a test topology
    // node0 --- node1
    //        |
    //        --- node2
    tgraph = create_new_graph("graph");
    struct node *tnode0 = create_graph_node(tgraph, "node0");
    struct node *tnode1 = create_graph_node(tgraph, "node1");
    struct node *tnode2 = create_graph_node(tgraph, "node2");
    insert_link_between_two_nodes(tnode0, tnode1, "eth01", "eth10", 1);
    insert_link_between_two_nodes(tnode0, tnode2, "eth02", "eth20", 1);
    node_set_intf_ip_addr(tnode0, "eth01", TEST_ETH01_IP_STR, 24);
    node_set_intf_ip_addr(tnode0, "eth02", "172.16.10.1", 16);
    node_set_intf_ip_addr(tnode1, "eth10", TEST_ETH10_IP_STR, 24);
    node_set_intf_ip_addr(tnode2, "eth20", "172.16.10.2", 16);
    reset_test_recv();
    set_receive_callback(receive_callback);
    return tgraph;
}

void destroy_test_topology()
{
    destroy_graph(tgraph);
    tgraph = NULL;
    set_receive_callback(NULL);
}
