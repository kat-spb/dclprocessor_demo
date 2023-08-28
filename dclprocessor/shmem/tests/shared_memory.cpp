#include "gtest/gtest.h"

extern "C" {
#include "shmem/shared_memory.h"
#include "shmem/memory_map.h"
}

#include <shmem/env_manipulations.h>

#include <cstdlib>

class SharedMemoryTest: public ::testing::Test {
protected:
    void SetUp() override {
        proc_cnt = 2;
        proc_size = 510;
        data_cnt = 1000;
        data_size = 2040;
        shmpath = strdup("/procsegment");
        map = shared_memory_map_create(proc_cnt, proc_size, data_cnt, data_size);
    }
    void TearDown() override {
        if(map) {
            free(map);
        }
    }
    int proc_cnt;
    size_t proc_size;
    int data_cnt;
    size_t data_size;
    char *shmpath;
    void *map;
};

TEST_F(SharedMemoryTest, map) {
    ASSERT_FALSE(map == nullptr);
    ASSERT_EQ(((memory_map_t *)map)->process_cnt, proc_cnt);
    ASSERT_EQ(((memory_map_t *)map)->process_size, proc_size);
    ASSERT_EQ(((memory_map_t *)map)->data_cnt, data_cnt);
    ASSERT_EQ(((memory_map_t *)map)->data_size, data_size);
}

TEST_F(SharedMemoryTest, get_size) {
    ASSERT_FALSE(map == nullptr);
    size_t estimated_size = MEMORY_MAP_SIZE +
                            MEMORY_ALIGN(proc_cnt * proc_size) +
                            MEMORY_ALIGN(data_cnt * data_size);
    ASSERT_EQ(shared_memory_size(map), estimated_size);
}

TEST_F(SharedMemoryTest, shared_memory_setenv) {
#if !defined(SHMEM_DATA_NAME_ENV) || !defined(SHMEM_DATA_SIZE_ENV)
    printf("SMSHMEM_DATA_NAME_ENV and SHMEM_DATA_SIZE_ENV expected to be defined");
    ASSERT_TRUE(FALSE);
#endif
    ASSERT_FALSE(map == nullptr);

    char *path_old = nullptr;
    char *size_old = nullptr;
    tmk::storeAndCleanEnv(&path_old, &size_old);

    int result = shared_memory_setenv(shmpath, map);
    char *path = getenv(SHMEM_DATA_NAME_ENV);
    if(path) {
        path = strdup(path);
    }
    char *size = getenv(SHMEM_DATA_SIZE_ENV);
    if(size) {
        size = strdup(size);
    }

    tmk::restoreEnv(&path_old, &size_old);

    if(path) {
        ASSERT_STREQ(path, shmpath);
        free(path);
    }
    else {
        ASSERT_TRUE(path != nullptr);
    }
    if(size) {
        ASSERT_EQ(std::atoi(size), shared_memory_size(map));
        free(size);
    }
    else {
        ASSERT_TRUE(size != nullptr);
    }
    ASSERT_EQ(result, 0);
}

TEST_F(SharedMemoryTest, shared_memory_getenv) {
#if !defined(SHMEM_DATA_NAME_ENV) || !defined(SHMEM_DATA_SIZE_ENV)
    printf("SMSHMEM_DATA_NAME_ENV and SHMEM_DATA_SIZE_ENV expected to be defined");
    ASSERT_TRUE(FALSE);
#endif
    ASSERT_FALSE(map == nullptr);

    char *path_old = nullptr;
    char *size_old = nullptr;
    tmk::storeAndCleanEnv(&path_old, &size_old);

    setenv(SHMEM_DATA_NAME_ENV, "/testsegment", 1);
    setenv(SHMEM_DATA_SIZE_ENV, "16651", 1);

    char *path;
    size_t bytes;
    int result = shared_memory_getenv(&path, &bytes);

    tmk::restoreEnv(&path_old, &size_old);

    ASSERT_STREQ(path, "/testsegment");
    free(path);
    ASSERT_EQ(bytes, 16651);
    ASSERT_EQ(result, 0);
}

namespace {
    void *createShmServerFailsafe(const char *shmpath, void *map) {
        void *shm =  shared_memory_server_init(shmpath, map);
        if(!shm) {
            shared_memory_unlink(shmpath);
            shm = shared_memory_server_init(shmpath, map);
        }
        return shm;
    }
}

TEST_F(SharedMemoryTest, shared_memory_server) {
    ASSERT_FALSE(map == nullptr);
    void *shm = createShmServerFailsafe(shmpath, map);
    ASSERT_FALSE(shm == nullptr);
    memory_map_t *testmap = (memory_map_t *)malloc(sizeof(memory_map_t));
    memmove(testmap, shm, sizeof(memory_map_t));
    ASSERT_EQ(testmap->process_cnt, proc_cnt);
    ASSERT_EQ(testmap->process_size, proc_size);
    ASSERT_EQ(testmap->data_cnt, data_cnt);
    ASSERT_EQ(testmap->data_size, data_size);
    free(testmap);
    shared_memory_destroy(shmpath, shm);
}

TEST_F(SharedMemoryTest, get_size_2) {
    ASSERT_FALSE(map == nullptr);
    void *shm = createShmServerFailsafe(shmpath, map);
    ASSERT_EQ(shared_memory_size(map), shared_memory_size(shm));
    shared_memory_destroy(shmpath, shm);
}

TEST_F(SharedMemoryTest, shared_memory_client) {
    ASSERT_FALSE(map == nullptr);
    void *shm1 = createShmServerFailsafe(shmpath, map);
    ASSERT_FALSE(shm1 == nullptr);
    void *shm2 = shared_memory_client_init(shmpath, shared_memory_size(shm1));
    ASSERT_FALSE(shm2 == nullptr);
    ASSERT_EQ(shared_memory_size(map), shared_memory_size(shm2));
    shared_memory_destroy(shmpath, shm1);
    shared_memory_destroy(shmpath, shm2);
}

TEST_F(SharedMemoryTest, shared_memory_write_read) {
    ASSERT_FALSE(map == nullptr);
    void *shm = createShmServerFailsafe(shmpath, map);
    std::string testbuff = "lorem ipsum dolor sit amet";
    std::string resultbuf(testbuff.length(), 0);
    shared_memory_write_test_data(shm, (void *)testbuff.data(), testbuff.length());
    shared_memory_read_test_data(shm, (void *)resultbuf.data(), testbuff.length());
    ASSERT_STREQ(testbuff.c_str(), resultbuf.c_str());
    shared_memory_destroy(shmpath, shm);
}