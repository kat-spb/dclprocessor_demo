#include "gtest/gtest.h"

extern "C" {
#include <description/description_json.h>
}

TEST(DCLDescriptionTest, SomeTest) {
    const char *expected = "Ok";
    const char *request = "Ok";
    ASSERT_STREQ(request, expected);
}

