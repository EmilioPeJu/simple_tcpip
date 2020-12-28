#include <string.h>
#include <unity.h>
#include "utils.h"

void setUp()
{
}

void tearDown()
{
}

void test_apply_mask()
{
    char *prefix = "172.16.10.20";
    char mask = 24;
    char result[16];
    apply_mask(prefix, mask, result);
    TEST_ASSERT_EQUAL(0, strcmp(result, "172.16.10.0"));
}

void test_convert_ip_from_str_to_int()
{
    TEST_ASSERT_EQUAL(16885952, convert_ip_from_str_to_int("192.168.1.1"));
}

void test_convert_ip_from_int_to_str()
{
    char result[16];
    convert_ip_from_int_to_str(16885952, result);
    TEST_ASSERT_EQUAL(0, strncmp(result, "192.168.1.1", 16));
}

void test_is_mac_broadcast_addr()
{
    struct mac_addr mac;
    layer2_fill_with_broadcast_mac(&mac);
    TEST_ASSERT(IS_MAC_BROADCAST_ADDR(&mac));
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_apply_mask);
    RUN_TEST(test_convert_ip_from_str_to_int);
    RUN_TEST(test_convert_ip_from_int_to_str);
    RUN_TEST(test_is_mac_broadcast_addr);
    return UNITY_END();
}
