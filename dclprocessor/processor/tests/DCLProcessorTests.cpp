#include "gtest/gtest.h"

extern "C" {
#include <processor/dclprocessor.h>
}

TEST(DCLProcessorTest, SomeTest) {
    const char *expected = "Ok";
    const char *request = "Ok";
    ASSERT_STREQ(request, expected);
}

