#include <unity.h>
#include "udp.h"

void setUp()
{
}

void tearDown()
{
}

void test_udp_hdr_size()
{
    TEST_ASSERT_EQUAL(8, UDP_HDR_SIZE);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_udp_hdr_size);
    return UNITY_END();
}
