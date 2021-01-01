#include <unity.h>
#include "icmp.h"

void setUp()
{
}

void tearDown()
{
}

void test_icmp_hdr_size()
{
    TEST_ASSERT_EQUAL(4, ICMP_HDR_SIZE);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_icmp_hdr_size);
    return UNITY_END();
}
