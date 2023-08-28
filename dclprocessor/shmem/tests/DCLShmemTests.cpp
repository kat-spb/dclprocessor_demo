#include "gtest/gtest.h"

extern "C" {
#include <shmem/shared_memory.h>
}

TEST(DCLShmemTest, SomeTest) {
    const char *expected = "Ok";
    const char *request = "Ok";
    ASSERT_STREQ(request, expected);
}

