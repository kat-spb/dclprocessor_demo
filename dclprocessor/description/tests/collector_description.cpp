#include "gtest/gtest.h"

extern "C" {
#include "description/collector_description.h"
}

TEST(collector_description, create) {
    struct collector_description *description = collector_description_create();
    ASSERT_FALSE(description == NULL);
    out_collector_description(description);
    collector_description_destroy(&description);
    ASSERT_TRUE(description == NULL);
}

//TODO: think about this test if function read will implemented
/*TEST(collector_description, read) {
    struct collector_description *description = NULL;
    read_collector_description(&description);
    ASSERT_FALSE(description == NULL);
    out_collector_description(description);
    free(description);
}*/
