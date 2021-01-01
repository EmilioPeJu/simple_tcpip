#include <unity.h>
#include "address.h"
#include "ethernet.h"
#include "arp.h"
#include "comm.h"
#include "fixture.h"
#include "graph.h"
#include "utils.h"

char *TEST_REQUEST =
        "\xff\xff\xff\xff\xff\xff\x00\x00\xe9\x08\x6e\x9a\x08\x06\x00\x01" \
        "\x08\x00\x06\x04\x00\x01\x00\x00\xe9\x08\x6e\x9a\xc0\xa8\x59\x01" \
                                "\x00\x00\x00\x00\x00\x00\xc0\xa8\x59\x02";
char *TEST_REQUEST2 =
        "\xff\xff\xff\xff\xff\xff\x00\x00\xe9\x08\x6e\x9a\x08\x06\x00\x01" \
        "\x08\x00\x06\x04\x00\x01\x00\x00\xe9\x08\x6e\x9a\xc0\xa8\x59\x01" \
                                "\x00\x00\x00\x00\x00\x00\xac\x10\x0a\x02";
char *TEST_REPLY =
        "\x00\x00\xe9\x08\x6e\x9a\x00\x30\xc3\x53\x50\xc8\x08\x06\x00\x01" \
        "\x08\x00\x06\x04\x00\x02\x00\x30\xc3\x53\x50\xc8\xc0\xa8\x59\x02" \
                                "\x00\x00\xe9\x08\x6e\x9a\xc0\xa8\x59\x01";
struct graph *tgraph;

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

void test_send_arp_broadcast_request_is_received()
{
    struct node *node0 = get_node_by_node_name(tgraph, "node0");
    struct node *node1 = get_node_by_node_name(tgraph, "node1");
    struct intf *eth01 = get_node_if_by_name(node0, "eth01");
    struct intf *eth10 = get_node_if_by_name(node1, "eth10");
    struct ip_addr ip;
    ip.iaddr = convert_ip_from_str_to_int("192.168.89.2");
    send_arp_broadcast_request(eth01, ip);
    wait_test_graph_received(1);
    // minimum ethernet frame size
    TEST_ASSERT_EQUAL_MESSAGE(60, get_test_recv_len(0),
                              "Size doesn't match");
    TEST_ASSERT_MESSAGE(memcmp(TEST_REQUEST, get_test_recv_data(0), 42) == 0,
                        "Packet doesn't match");
    TEST_ASSERT_MESSAGE(eth10 == get_test_recv_intf(0),
                        "Interface doesn't match");
}

void test_process_arp_broadcast_request_does_reply()
{
    struct node *node0 = get_node_by_node_name(tgraph, "node0");
    struct node *node1 = get_node_by_node_name(tgraph, "node1");
    struct intf *eth01 = get_node_if_by_name(node0, "eth01");
    struct intf *eth10 = get_node_if_by_name(node1, "eth10");
    process_arp_broadcast_request(eth10,
        (struct arp_hdr *) (TEST_REQUEST + ETH_HDR_SIZE));
    wait_test_graph_received(1);
    // minimum ethernet frame size
    TEST_ASSERT_EQUAL_MESSAGE(60, get_test_recv_len(0),
                              "Size doesn't match");
    TEST_ASSERT_MESSAGE(memcmp(TEST_REPLY, get_test_recv_data(0), 42) == 0,
                        "Packet doesn't match");
    TEST_ASSERT_MESSAGE(eth01 == get_test_recv_intf(0),
                        "Interface doesn't match");
}

void test_process_arp_broadcast_request_filters()
{
    struct node *node1 = get_node_by_node_name(tgraph, "node1");
    struct intf *eth10 = get_node_if_by_name(node1, "eth10");

    TEST_ASSERT_FALSE(process_arp_broadcast_request(eth10,
        (struct arp_hdr *) (TEST_REQUEST2 + ETH_HDR_SIZE)));
}

void test_process_arp_reply_msg_resolves_arp_entry()
{
    struct node *node0 = get_node_by_node_name(tgraph, "node0");
    struct node *node1 = get_node_by_node_name(tgraph, "node1");
    struct intf *eth01 = get_node_if_by_name(node0, "eth01");
    struct intf *eth10 = get_node_if_by_name(node1, "eth10");
    struct arp_entry *entry, *unresolved_entry;
    unresolved_entry = calloc(1, sizeof(struct arp_entry));
    struct ip_addr ip;
    stop_recv_thread();
    ip.iaddr = convert_ip_from_str_to_int("192.168.89.2");
    init_arp_entry(unresolved_entry, ip, eth01);
    arp_table_entry_add(NODE_ARP_TABLE(node0), unresolved_entry);
    TEST_ASSERT_MESSAGE(process_arp_reply_msg(eth01,
                            (struct arp_hdr *) (TEST_REPLY + ETH_HDR_SIZE)),
                        "arp message not processed");
    entry = arp_table_lookup(NODE_ARP_TABLE(node0), ip);
    TEST_ASSERT_MESSAGE(entry->resolved, "Arp entry not marked as resolved");
    TEST_ASSERT_MESSAGE(memcmp(entry->mac.addr, IF_MAC(eth10)->addr,
                               MAC_ADDR_SIZE) == 0,
                        "Arp entry not resolved");
}

void test_arp_header_size()
{
    TEST_ASSERT_EQUAL(28, ARP_HDR_SIZE);
}

void test_arp_table_add_and_lookup()
{
    struct arp_table table;
    struct arp_entry entry1;
    struct intf intf1;
    struct ip_addr ip1, ip2;
    init_arp_table(&table);
    memcpy(ip1.addr, "\x01\x02\x03\x04", IP_ADDR_SIZE);
    memcpy(ip2.addr, "\x01\x02\x03\x05", IP_ADDR_SIZE);
    memcpy(entry1.ip.addr, ip1.addr, IP_ADDR_SIZE);
    memcpy(entry1.mac.addr, "\x01\x02\x03\x04\x05\x06", MAC_ADDR_SIZE);
    entry1.intf = &intf1;
    TEST_ASSERT_MESSAGE(arp_table_entry_add(&table, &entry1),
                        "Error adding arp entry");
    TEST_ASSERT_MESSAGE(arp_table_lookup(&table, ip1) == &entry1,
                        "Error looking up arp entry");
    TEST_ASSERT_MESSAGE(arp_table_lookup(&table, ip2) == NULL,
                        "Shouldn't find inexistant entry");
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_send_arp_broadcast_request_is_received);
    RUN_TEST(test_arp_header_size);
    RUN_TEST(test_process_arp_broadcast_request_does_reply);
    RUN_TEST(test_process_arp_broadcast_request_filters);
    RUN_TEST(test_process_arp_reply_msg_resolves_arp_entry);
    RUN_TEST(test_arp_table_add_and_lookup);
    return UNITY_END();
}
