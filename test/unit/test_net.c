#include <string.h>
#include <unity.h>
#include "graph.h"
#include "net.h"
#define TEST_IP "192.168.0.1"
#define TEST_IP_RAW "\xc0\xa8\x00\x01"
#define TEST_MASK (24)

struct graph *tgraph;
struct node *tnode0;
struct node *tnode1;
struct node *tnode2;

void setUp()
{
    // set up a test graph
    // node1 <---> node2
    tgraph = create_new_graph("graph");
    tnode0 = create_graph_node(tgraph, "node_0");
    tnode1 = create_graph_node(tgraph, "node_1");
    tnode2 = create_graph_node(tgraph, "node_2");
    insert_link_between_two_nodes(tnode0, tnode1, "eth01", "eth10", 1);
    insert_link_between_two_nodes(tnode0, tnode2, "eth02", "eth20", 1);
    node_set_intf_ip_addr(tnode0, "eth01", "192.168.89.1", 24);
    node_set_intf_ip_addr(tnode0, "eth02", "172.16.10.1", 16);
}

void tearDown()
{
    destroy_node(tnode1);
    destroy_node(tnode2);
    destroy_graph(tgraph);
}

void test_init_intf_nw_prop()
{
    struct intf_nw_prop props;
    memset((char *) &props, 1, sizeof(props));
    init_intf_nw_prop(&props);
    TEST_ASSERT_EQUAL_MESSAGE(0, props.is_ip_addr_config,
                              "ip flag not 0");
    TEST_ASSERT_EQUAL_MESSAGE(0,
        memcmp(props.ip_addr.addr, "\x00\x00\x00\x00", 4),
        "IP address not resetted"
    );
    TEST_ASSERT_EQUAL_MESSAGE(0, props.mask, "Mask not resetted");
}

void test_init_node_nw_prop()
{
    struct node_nw_prop props;
    memset((char *) &props, 1, sizeof(props));
    init_node_nw_prop(&props);
    TEST_ASSERT_EQUAL_MESSAGE(0, props.is_lb_addr_config,
                              "loopback flag not 0");
    TEST_ASSERT_EQUAL_MESSAGE(0,
        memcmp(props.lb_addr.addr, "\x00\x00\x00\x00", IP_ADDR_SIZE),
        "IP address not resetted"
    );
}


void test_node_set_loopback_address_with_valid_address()
{
    struct node node1;
    node_set_loopback_address(&node1, TEST_IP);
    TEST_ASSERT_EQUAL(0,
        memcmp(node1.node_nw_prop.lb_addr.addr, TEST_IP_RAW, 4));
    TEST_ASSERT_EQUAL(true, node1.node_nw_prop.is_lb_addr_config);
}

void test_set_intf_ip_address_with_valid_address()
{
    node_set_intf_ip_addr(tnode0, "eth01", TEST_IP, TEST_MASK);
    TEST_ASSERT_EQUAL(0,
        memcmp(IF_IP(tnode0->intfs[0]), TEST_IP_RAW, 4));
    TEST_ASSERT_EQUAL(true, tnode0->intfs[0]->intf_nw_prop.is_ip_addr_config);
    TEST_ASSERT_EQUAL(TEST_MASK, tnode0->intfs[0]->intf_nw_prop.mask);
}

void test_get_matching_subnet_interface_with_matching_if()
{
    TEST_ASSERT_MESSAGE(
        get_matching_subnet_interface(tnode0,"192.168.89.20") == tnode0->intfs[0],
        "First interface not found");
    TEST_ASSERT_MESSAGE(
        get_matching_subnet_interface(tnode0, "172.16.90.100") == tnode0->intfs[1],
        "Second interface not found");
}

void test_get_matching_subnet_interface_without_matching_if()
{
    TEST_ASSERT(
        get_matching_subnet_interface(tnode0,"10.20.30.40") == NULL);
    TEST_ASSERT(
        get_matching_subnet_interface(tnode0,"192.168.90.20") == NULL);
    TEST_ASSERT(
        get_matching_subnet_interface(tnode0, "172.17.90.100") == NULL);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_init_intf_nw_prop);
    RUN_TEST(test_node_set_loopback_address_with_valid_address);
    RUN_TEST(test_set_intf_ip_address_with_valid_address);
    RUN_TEST(test_get_matching_subnet_interface_with_matching_if);
    RUN_TEST(test_get_matching_subnet_interface_without_matching_if);
    return UNITY_END();
}
