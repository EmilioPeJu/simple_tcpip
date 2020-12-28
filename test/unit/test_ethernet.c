#include <unity.h>
#include "graph.h"
#include "ethernet.h"
#include "fixture.h"

struct graph *tgraph;
struct node *tnode0;
struct node *tnode1;
struct node *tnode2;

void setUp()
{
    tgraph = create_test_topology();
}

void tearDown()
{
    destroy_test_topology();
    tgraph = NULL;
}

void test_ethernet_input_qualify_true_cases()
{
    char *pkt1 = "\xff\xff\xff\xff\xff\xff\xdc\xa6\x32\x04\xdd\x0a\x08\x00";
    char *pkt2 = "\x00\x00\xe9\x08\x6e\x9a\xdc\xa6\x32\x04\xdd\x0a\x08\x00";
    struct node *tnode0 = get_node_by_node_name(tgraph, "node0");
    struct intf *intf = get_node_if_by_name(tnode0, "eth01");
    TEST_ASSERT_EQUAL(true,
        ethernet_input_qualify(intf, (struct ethernet_hdr *) pkt1));
    TEST_ASSERT_EQUAL(true,
        ethernet_input_qualify(intf, (struct ethernet_hdr *) pkt2));
}

void test_ethernet_input_qualify_false_cases()
{
    char *pkt1 = "\xf1\xf2\xf3\xf4\xf5\xf6\xdc\xa6\x32\x04\xdd\x0a\x08\x00";
    char *pkt2 = "\x00\x00\x00\x00\x00\x00\xdc\xa6\x32\x04\xdd\x0a\x08\x00";
    struct node *tnode0 = get_node_by_node_name(tgraph, "node0");
    struct intf *intf = get_node_if_by_name(tnode0, "eth01");
    TEST_ASSERT_EQUAL(false,
        ethernet_input_qualify(intf, (struct ethernet_hdr *) pkt1));
    TEST_ASSERT_EQUAL(false,
        ethernet_input_qualify(intf, (struct ethernet_hdr *) pkt2));
}

void test_ethernet_header_size()
{
    TEST_ASSERT_EQUAL(14, ETH_HDR_SIZE);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_ethernet_input_qualify_true_cases);
    RUN_TEST(test_ethernet_input_qualify_false_cases);
    RUN_TEST(test_ethernet_header_size);
    return UNITY_END();
}
