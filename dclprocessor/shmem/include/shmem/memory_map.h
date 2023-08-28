#ifndef _MEMORY_MAP_H_
#define _MEMORY_MAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MEMORY_ALIGN(size)      ((((size) + 255) >> 8) << 8)
#define MEMORY_MAP_SIZE         MEMORY_ALIGN(sizeof(memory_map_t))

typedef struct memory_map {
    //process and data parts are used and necessary for pipeline
    int process_cnt;
    size_t process_size;
    int data_cnt;
    size_t data_size;
    //description part could be used in the any part of project
    int description_zone_cnt;
    size_t description_zone_size;
} memory_map_t;

static inline void *memory_map_create(int proc_cnt, size_t proc_size, int data_cnt, size_t data_size, int desc_cnt, size_t desc_size) {
    memory_map_t *map = (memory_map_t *)malloc(sizeof(memory_map_t));
    map->process_cnt = proc_cnt;
    map->process_size = proc_size;
    map->data_cnt = data_cnt;
    map->data_size = data_size;
    map->description_zone_cnt = desc_cnt;
    map->description_zone_size = desc_size;
    return (void*)map;
}

static inline size_t memory_get_size(memory_map_t *map){
    return MEMORY_MAP_SIZE +
       MEMORY_ALIGN(map->process_size * map->process_cnt) +
       MEMORY_ALIGN(map->data_size * map->data_cnt) +
       MEMORY_ALIGN(map->description_zone_cnt * map->description_zone_size);
}

static inline size_t memory_get_process_size(void *shmem){
    memory_map_t *map = (memory_map_t *)shmem;
    return MEMORY_ALIGN(map->process_size * map->process_cnt);
}

static inline size_t memory_get_data_size(void *shmem){
    memory_map_t *map = (memory_map_t *)shmem;
    return MEMORY_ALIGN(map->data_size * map->data_cnt);
}

static inline size_t memory_get_description_size(void *shmem){
    memory_map_t *map = (memory_map_t *)shmem;
    return MEMORY_ALIGN(map->description_zone_cnt * map->description_zone_size);
}

static inline size_t memory_get_map_offset(void *shmem){
    (void)shmem;
    return 0;
}

//IMPORTANT: process_and_data part is 'one whole'
static inline size_t memory_get_process_offset(void *shmem){
    (void)shmem;
    return MEMORY_MAP_SIZE;
}

static inline size_t memory_get_data_offset(void *shmem){
    memory_map_t *map = (memory_map_t *)shmem;
    return MEMORY_MAP_SIZE +
       MEMORY_ALIGN(map->process_size * map->process_cnt);
}

//IMPORTANT: description should be after process_and_data part
static inline size_t memory_get_description_offset(void *shmem){
    memory_map_t *map = (memory_map_t *)shmem;
    return MEMORY_MAP_SIZE +
       MEMORY_ALIGN(map->process_cnt * map->process_size) +
       MEMORY_ALIGN(map->data_cnt * map->data_size);
}

#ifdef __cplusplus
}
#endif

#endif //_MEMORY_MAP_H_

