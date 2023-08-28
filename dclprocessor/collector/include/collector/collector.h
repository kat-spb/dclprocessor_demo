#ifndef _COLLECTOR_H_
#define _COLLECTOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <semaphore.h>
#include <pthread.h>

#include <misc/ring_buffer.h>

#include <collector/source.h>
#include <description/collector_description.h>

#define MAX_SOURCES_NUMBER 5

struct data_collector {
    int exit_flag;
    pthread_t thread_id;
    int sources_cnt;
    struct data_source *sources[MAX_SOURCES_NUMBER];
    //struct data_source **sources;
    sem_t empty_ready;
    sem_t data_ready;

    char data[MAX_SHMEM_NAME_LEN];  //current capturing obj_name

    void *data_memory; //for example second ring in pipeline data memory or local process memory
    size_t dataoffset;
    size_t chunk_cnt;
    size_t chunk_size;
    struct ring_buffer *data_ring;  //ring of objects

    int use_boost_shmem;
    char boost_path[MAX_SHMEM_NAME_LEN];
    size_t boost_size;
};

struct data_collector *collector_create();
void collector_set_boost_shmem(struct data_collector *collector, int use_boost_shmem, char *boost_path, size_t boost_size);
struct data_collector *collector_init(struct collector_description *desc);
void collector_destroy(struct data_collector *collector);

#ifdef __cplusplus
}
#endif

#endif //_COLLECTOR_H_
