#include "gtest/gtest.h"

extern "C" {
#include "shmem/memory_map.h"
}

TEST(memory_map, defines) {
#ifndef MEMORY_ALIGN
    printf("MEMORY_ALIGN expected to be defined");
    ASSERT_FALSE(true);
#else
    size_t size = 1234;
    size_t aligned = MEMORY_ALIGN(size);
    ASSERT_EQ(aligned, ((((size) + 255) >> 8) << 8));
#endif

#ifndef MEMORY_MAP_SIZE
    printf("MEMORY_ALIGN expected to be defined");
    ASSERT_FALSE(true);
#else
    size = sizeof(memory_map_t);
    ASSERT_EQ(MEMORY_MAP_SIZE, ((((size) + 255) >> 8) << 8));
#endif

}

class MemoryMapTest: public ::testing::Test {
protected:
    void SetUp() override {
        proc_cnt = 2;
        proc_size = 510;
        data_cnt = 1000;
        data_size = 2040;
        desc_cnt = 2;
        desc_size = 0xFFFF; //64Kb
        //ATTENTION: inteface changed
        map = (memory_map_t *)memory_map_create(proc_cnt, proc_size, data_cnt, data_size, desc_cnt, desc_size);
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
    int desc_cnt;
    size_t desc_size;
    memory_map_t *map;
};

TEST_F(MemoryMapTest, created) {
    ASSERT_FALSE(map == nullptr);
    ASSERT_EQ(map->process_cnt, proc_cnt);
    ASSERT_EQ(map->process_size, proc_size);
    ASSERT_EQ(map->data_cnt, data_cnt);
    ASSERT_EQ(map->data_size, data_size);
    ASSERT_EQ(map->description_zone_cnt, desc_cnt);
    ASSERT_EQ(map->description_zone_size, desc_size);
}

TEST_F(MemoryMapTest, get_size) {
    ASSERT_FALSE(map == nullptr);
    size_t estimated_size = MEMORY_MAP_SIZE +
                            MEMORY_ALIGN(proc_cnt * proc_size) +
                            MEMORY_ALIGN(data_cnt * data_size) +
                            MEMORY_ALIGN(desc_cnt * desc_size);
    ASSERT_EQ(memory_get_size(map), estimated_size);
}

TEST_F(MemoryMapTest, get_map_offset) {
    ASSERT_FALSE(map == nullptr);
    size_t estimated_offset = 0;
    ASSERT_EQ(memory_get_map_offset(map), estimated_offset);
}

TEST_F(MemoryMapTest, get_process_offset) {
    ASSERT_FALSE(map == nullptr);
    size_t estimated_offset = MEMORY_MAP_SIZE;
    ASSERT_EQ(memory_get_process_offset(map), estimated_offset);
}

TEST_F(MemoryMapTest, get_data_offset) {
    size_t estimated_offset = MEMORY_MAP_SIZE + MEMORY_ALIGN(proc_cnt * proc_size);
    ASSERT_EQ(memory_get_data_offset(map), estimated_offset);
}

TEST_F(MemoryMapTest, get_data_size) {
    ASSERT_FALSE(map == nullptr);
    size_t estimated_size = MEMORY_ALIGN(data_cnt * data_size);
    ASSERT_EQ(memory_get_data_size(map), estimated_size);
}

TEST_F(MemoryMapTest, get_description_offset) {
    size_t estimated_offset = MEMORY_MAP_SIZE + MEMORY_ALIGN(proc_cnt * proc_size) + MEMORY_ALIGN(data_cnt * data_size);
    ASSERT_EQ(memory_get_description_offset(map), estimated_offset);
}

TEST_F(MemoryMapTest, get_description_size) {
    ASSERT_FALSE(map == nullptr);
    size_t estimated_size = MEMORY_ALIGN(desc_cnt * desc_size);
    ASSERT_EQ(memory_get_description_size(map), estimated_size);
}


TEST_F(MemoryMapTest, get_size_2) {
    ASSERT_FALSE(map == nullptr);
    size_t estimated_size = memory_get_description_offset(map) + memory_get_description_size(map);
    ASSERT_EQ(memory_get_size(map), estimated_size);
}
