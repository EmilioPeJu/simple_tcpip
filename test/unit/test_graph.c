#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graph.h>
#include <unity.h>

#define N_TNODES (5)
#define N_TLINKS (20)
static struct node tnodes[N_TNODES];
static struct link tlinks[N_TLINKS];

void init_test_graph()
{
    for (int i=0; i < N_TNODES; i++) {
        memset(&tnodes[i], 0, sizeof(tnodes[i]));
    }
    for (int i=0; i < N_TLINKS; i++) {
        memset(&tlinks[i], 0, sizeof(tlinks[i]));
    }
    // connect node0 <--> node1
    tlinks[0].intf1.node = &tnodes[0];
    tlinks[0].intf2.node = &tnodes[1];
    tlinks[0].intf1.link = &tlinks[0];
    tlinks[0].intf2.link = &tlinks[0];
    tnodes[0].intfs[0] = &tlinks[0].intf1;
    tnodes[1].intfs[0] = &tlinks[0].intf2;
    // connect  node0 <--> node2
    tlinks[1].intf1.node = &tnodes[0];
    tlinks[1].intf2.node = &tnodes[2];
    tlinks[1].intf1.link = &tlinks[1];
    tlinks[1].intf2.link = &tlinks[1];
    tnodes[0].intfs[1] = &tlinks[0].intf1;
    tnodes[2].intfs[0] = &tlinks[0].intf2;
}

void setUp()
{
    init_test_graph();
}

void tearDown()
{
}

void test_get_nbr_node_with_link()
{
    struct intf *intf1 = &tlinks[0].intf1;
    struct intf *intf2 = &tlinks[0].intf2;
    struct node *node = get_nbr_node(intf1);
    TEST_ASSERT(node == &tnodes[1]);
    node = get_nbr_node(intf2);
    TEST_ASSERT(node == &tnodes[0]);
}

void test_get_nbr_node_without_link()
{
    struct intf intf;
    memset(&intf, 0, sizeof(intf));
    struct node *node = get_nbr_node(&intf);
    TEST_ASSERT(node == NULL);
}

void test_get_nbr_node_without_node()
{
    tlinks[0].intf1.node = NULL;
    tlinks[0].intf2.node = NULL;
    struct intf *intf = &tlinks[0].intf1;
    struct node *node = get_nbr_node(intf);
    TEST_ASSERT(node == NULL);
    intf = &tlinks[0].intf2;
    node = get_nbr_node(intf);
    TEST_ASSERT(node == NULL);
}

void test_get_intf_available_slot_with_interfaces()
{
    TEST_ASSERT_EQUAL(get_node_intf_available_slot(&tnodes[1]), 1);
    TEST_ASSERT_EQUAL(get_node_intf_available_slot(&tnodes[0]), 2);
}

void test_get_intf_available_slot_without_interfaces()
{
    struct node node;
    memset(&node, 0, sizeof(node));
    TEST_ASSERT_EQUAL(get_node_intf_available_slot(&node), 0);
}

void test_get_intf_available_slot_without_slots()
{
    struct node node;
    struct intf intf;
    memset(&node, 0, sizeof(node));
    for (int i=0; i < MAX_INTFS_PER_NODE; i++)
        node.intfs[i] = &intf;
    TEST_ASSERT_EQUAL(get_node_intf_available_slot(&node), -1);
}

void test_create_new_graph()
{
    struct graph *graph = create_new_graph("test_graph");
    TEST_ASSERT(graph != NULL);
    TEST_ASSERT_EQUAL_MESSAGE(strcmp(graph->name, "test_graph"), 0,
                              "name doesn't match");
    TEST_ASSERT_MESSAGE(list_empty(&graph->nodes), "list not empty");
    destroy_graph(graph);
}

void test_create_graph_node()
{
    struct graph *graph = create_new_graph("test_graph");
    struct node *node = create_graph_node(graph, "test_node");
    TEST_ASSERT_EQUAL_MESSAGE(strcmp(node->name, "test_node"), 0,
                              "name doesn't match");
    TEST_ASSERT_MESSAGE(graph->nodes.next == &node->list,
                        "node not in graph");
    destroy_node(node);
    TEST_ASSERT_MESSAGE(list_empty(&graph->nodes),
                        "list not empty after deleting node");
    destroy_graph(graph);
}

void test_insert_link_between_two_nodes()
{
    struct graph *graph = create_new_graph("test_graph");
    struct node *node1 = create_graph_node(graph, "test_node1");
    struct node *node2 = create_graph_node(graph, "test_node2");
    insert_link_between_two_nodes(node1, node2, "if1", "if2", 42);
    TEST_ASSERT_MESSAGE(strcmp(node1->intfs[0]->name, "if1") == 0,
                        "interface name of first node doesn't match");
    TEST_ASSERT_MESSAGE(strcmp(node2->intfs[0]->name, "if2") == 0,
                        "interface name of second node doesn't match");
    TEST_ASSERT_EQUAL_MESSAGE(node1->intfs[0]->link->cost, 42,
                              "link cost doesn't match");
    TEST_ASSERT_MESSAGE(node1->intfs[0]->link == node2->intfs[0]->link,
                        "link of connected interfaces aren't the same");
    TEST_ASSERT_MESSAGE(node1->intfs[0]->node == node1,
                "interface doesn't point to node");
    TEST_ASSERT_MESSAGE(node2->intfs[0]->node == node2,
                "interface doesn't point to node");
    destroy_node(node1);
    destroy_node(node2);
    destroy_graph(graph);
}

void test_get_node_if_by_name()
{
    struct graph *graph = create_new_graph("test_graph");
    struct node *node1 = create_graph_node(graph, "test_node1");
    struct node *node2 = create_graph_node(graph, "test_node2");
    insert_link_between_two_nodes(node1, node2, "if1", "if2", 42);
    TEST_ASSERT_MESSAGE(get_node_if_by_name(node1, "if1") == node1->intfs[0],
                        "first interface not found");
    TEST_ASSERT_MESSAGE(get_node_if_by_name(node2, "if2") == node2->intfs[0],
                        "second interface not found");
    destroy_node(node1);
    destroy_node(node2);
    destroy_graph(graph);
}

void test_get_node_by_node_name()
{
    struct graph *graph = create_new_graph("test_graph");
    struct node *node1 = create_graph_node(graph, "test_node1");
    struct node *node2 = create_graph_node(graph, "test_node2");
    TEST_ASSERT_MESSAGE(get_node_by_node_name(graph, "test_node1") == node1,
                       "node doesn't match");
    TEST_ASSERT_MESSAGE(get_node_by_node_name(graph, "test_node2") == node2,
                       "node doesn't match");
    destroy_node(node1);
    destroy_node(node2);
    destroy_graph(graph);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_get_nbr_node_with_link);
    RUN_TEST(test_get_nbr_node_without_link);
    RUN_TEST(test_get_intf_available_slot_with_interfaces);
    RUN_TEST(test_get_intf_available_slot_without_interfaces);
    RUN_TEST(test_get_intf_available_slot_without_slots);
    RUN_TEST(test_create_new_graph);
    RUN_TEST(test_create_graph_node);
    RUN_TEST(test_insert_link_between_two_nodes);
    RUN_TEST(test_get_node_if_by_name);
    RUN_TEST(test_get_node_by_node_name);
    return UNITY_END();
}
