#include "gtest/gtest.h"

extern "C" {
#include <pipeline/pipeline.h>
}

TEST(DCLPipelineTest, SomeTest) {
    const char *expected = "Ok";
    const char *request = "Ok";
    ASSERT_STREQ(request, expected);
}

