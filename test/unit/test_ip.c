#include <stdlib.h>
#include <unity.h>
#include "comm.h"
#include "graph.h"
#include "fixture.h"
#include "icmp.h"
#include "ip.h"
#include "utils.h"
struct graph *tgraph;
char *example_ip_hdr = "\x45\x00\x00\x73\x00\x00\x40\x00\x40\x11\xb8\x61\xc0" \
                       "\xa8\x00\x01\xc0\xa8\x00\xc7";
const char *example_ip_hdr_no_cksum = \
    "\x45\x00\x00\x73\x00\x00\x40\x00\x40\x11\x00\x00\xc0\xa8\x00\x01\xc0" \
    "\xa8\x00\xc7";

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

void test_calc_ip_hcksum()
{
    u16 result = calc_ip_hcksum((struct ip_hdr *) example_ip_hdr_no_cksum);
    TEST_ASSERT_EQUAL(0xb861, result);
}

void test_fill_ip_hcksum()
{
    char buff[32];
    memcpy(buff, example_ip_hdr_no_cksum, 20);
    struct ip_hdr *hdr = (struct ip_hdr *) buff;
    fill_ip_hcksum(hdr);
    TEST_ASSERT(check_ip_hcksum(hdr));
}

void test_check_ip_hcksum()
{
    TEST_ASSERT(check_ip_hcksum((struct ip_hdr *) example_ip_hdr));
}


void test_ping_pong()
{
    struct node *node0 = get_node_by_node_name(tgraph, "node0");
    struct node *node1 = get_node_by_node_name(tgraph, "node1");
    struct intf *eth01 = get_node_if_by_name(node0, "eth01");
    struct intf *eth10 = get_node_if_by_name(node1, "eth10");
    struct arp_entry *entry0, *entry1;
    entry0 = calloc(1, sizeof(struct arp_entry));
    entry1 = calloc(1, sizeof(struct arp_entry));
    // required ARP entries
    // entry for node0
    init_arp_entry(entry0, *IF_IP(eth10), eth01);
    memcpy(entry0->mac.addr, IF_MAC(eth10)->addr, MAC_ADDR_SIZE);
    entry0->resolved = true;
    arp_table_entry_add(NODE_ARP_TABLE(node0), entry0);
    // entry for node1
    init_arp_entry(entry1, *IF_IP(eth01), eth10);
    memcpy(entry1->mac.addr, IF_MAC(eth01)->addr, MAC_ADDR_SIZE);
    entry1->resolved = true;
    arp_table_entry_add(NODE_ARP_TABLE(node1), entry1);
    dump_nw_graph(tgraph);
    ping(node0, TEST_ETH10_IP_STR);
    wait_test_graph_received(2);
    TEST_ASSERT_EQUAL(2, get_test_recv_n());
    // ping was received by node1 via eth10
    TEST_ASSERT(get_test_recv_intf(0) == eth10);
    // pong was received by node0 via eth01
    TEST_ASSERT(get_test_recv_intf(1) == eth01);
}

void test_ip_hdr_size()
{
    TEST_ASSERT_EQUAL(20, IP_HDR_SIZE);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_ip_hdr_size);
    RUN_TEST(test_ping_pong);
    RUN_TEST(test_calc_ip_hcksum);
    RUN_TEST(test_check_ip_hcksum);
    RUN_TEST(test_fill_ip_hcksum);
    return UNITY_END();
}
