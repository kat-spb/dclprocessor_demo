#include "gtest/gtest.h"

extern "C" {
#include "description/source_description.h"
}

TEST(source_description, create) {
    struct source_description *description = source_description_create();
    ASSERT_FALSE(description == NULL);
    out_source_description(description);
    source_description_destroy(&description);
    ASSERT_TRUE(description == NULL);
}

//TODO: think about this test after function read will implemented
/*TEST(source_description, read) {
    struct source_description *description = NULL;
    read_source_description(&description);
    ASSERT_FALSE(description == NULL);
    out_source_description(description);
    free(description);
}*/