#include <string.h>
#include <unity.h>
#include "tcp.h"
#include "utils.h"
const char *TEST_TCP_PACKET = \
    "\xa6\x3e\x01\xbb\x41\x5f\xc2\xc2\x29\x42\x3f\x2c\x80\x10\x01\xf5" \
    "\x00\x00\x00\x00\x01\x01\x08\x0a\x9e\x7f\x07\x44\x9b\x21\x01\x04";
const size_t TEST_TCP_PACKET_SZ = 32;


void setUp()
{
}

void tearDown()
{
}

void test_tcp_hdr_size()
{
    TEST_ASSERT_EQUAL(20, TCP_HDR_SIZE);
    TEST_ASSERT_EQUAL(12, PSEUDO_IP_HDR_SIZE);
}

void test_tcp_cksum()
{
    char buffer[128];
    struct ip_addr src_ip;
    struct ip_addr dst_ip;
    src_ip.iaddr = convert_ip_from_str_to_int("192.168.1.229");
    dst_ip.iaddr = convert_ip_from_str_to_int("185.199.109.153");
    memcpy(buffer + 12, TEST_TCP_PACKET, TEST_TCP_PACKET_SZ);
    u16 result = calc_tcp_cksum((struct tcp_hdr *) (buffer + 12),
                                TEST_TCP_PACKET_SZ,
                                src_ip, dst_ip);
    TEST_ASSERT_EQUAL(0x3467, result);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_tcp_hdr_size);
    RUN_TEST(test_tcp_cksum);
    return UNITY_END();
}
