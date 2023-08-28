#include "gtest/gtest.h"

extern "C" {
#include "pipeline/pipeline.h"
}

#include <shmem/env_manipulations.h>

TEST(pipeline, init_null) {
    ASSERT_DEATH({pipeline_init(nullptr);},"");
}

TEST(pipeline, DISABLED_init_default) {
    char *path_old = nullptr;
    char *size_old = nullptr;
    tmk::storeAndCleanEnv(&path_old, &size_old);
    struct pipeline *pipeLine = pipeline_init("default");
    tmk::restoreEnv(&path_old, &size_old);
    ASSERT_FALSE(pipeLine == nullptr);
    pipeline_destroy(pipeLine);
}
