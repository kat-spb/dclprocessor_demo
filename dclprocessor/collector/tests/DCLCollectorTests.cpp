#include "gtest/gtest.h"

extern "C" {
#include "collector/collector.h"
}

TEST(DCLCollectorTest, SomeTest) {
    const char *expected = "Ok";
    const char *request = "Ok";
    ASSERT_STREQ(request, expected);
}

