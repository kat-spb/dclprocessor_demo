#include "gtest/gtest.h"

extern "C" {
#include <process/process_mngr.h>
}

TEST(DCLProcessTest, SomeTest) {
    const char *expected = "Ok";
    const char *request = "Ok";
    ASSERT_STREQ(request, expected);
}

