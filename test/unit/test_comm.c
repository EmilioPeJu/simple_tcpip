#include <unistd.h>
#include <unity.h>
#include "comm.h"
#include "graph.h"
#include "fixture.h"

static struct graph *tgraph;

void setUp()
{
    tgraph = create_test_topology();
    start_recv_thread();
}

void tearDown()
{
    stop_recv_thread();
    destroy_test_topology();
    tgraph = NULL;
}

static void send_receive(char *test_data, size_t test_data_len,
                         size_t frame_i, struct intf *oif, struct intf *iif)
{
    send_bytes_out(test_data, test_data_len, oif);
    wait_test_graph_received(frame_i + 1);
    TEST_ASSERT_EQUAL_MESSAGE(0, memcmp(test_data, get_test_recv_data(frame_i),
                                        test_data_len),
                              "Message doesn't match");
    TEST_ASSERT_EQUAL_MESSAGE(test_data_len, get_test_recv_len(frame_i), "Lengh doesn't match");
    TEST_ASSERT_MESSAGE(iif == get_test_recv_intf(frame_i),
                        "Interface doesn't match");
}

void test_send_receive_once()
{
    struct node *tnode0 = get_node_by_node_name(tgraph, "node0");
    struct node *tnode1 = get_node_by_node_name(tgraph, "node1");
    struct intf *eth01 = get_node_if_by_name(tnode0, "eth01");
    struct intf *eth10 = get_node_if_by_name(tnode1, "eth10");
    send_receive("\x01\x02\x03\x04\x05", 5, 0, eth01, eth10);
}

void test_send_receive_many_times()
{
    struct node *tnode0 = get_node_by_node_name(tgraph, "node0");
    struct node *tnode1 = get_node_by_node_name(tgraph, "node1");
    struct node *tnode2 = get_node_by_node_name(tgraph, "node2");
    struct intf *eth01 = get_node_if_by_name(tnode0, "eth01");
    struct intf *eth02 = get_node_if_by_name(tnode0, "eth02");
    struct intf *eth10 = get_node_if_by_name(tnode1, "eth10");
    struct intf *eth20 = get_node_if_by_name(tnode2, "eth20");
    send_receive("\x01\x02\x03\x04\x05", 5, 0, eth01, eth10);
    send_receive("\x11\x22\x33\x44\x55\x66\x77\x88\x99", 9, 1, eth01, eth10);
    send_receive("\x88\x88\x88\x88\x88", 5, 2, eth02, eth20);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_send_receive_once);
    RUN_TEST(test_send_receive_many_times);
    return UNITY_END();
}
