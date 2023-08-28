#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/uio.h>

#include <semaphore.h>
#include <pthread.h>

#include <collector/source.h>
#include <collector/collector.h>
#include <description/collector_description.h>

void *collect_data_thread(void *arg){

    struct data_collector *collector = (struct data_collector*)arg;

    //fprintf(stdout, "[collect_data_thread]: I'm  in collector thread before wait(collector:empty)\n");
    //fflush(stdout);
    sem_wait(&collector->empty_ready);
    //printf(stdout, "[collect_data_thread]: I'm  in collector thread after wait(collector:empty)\n");
    //fflush(stdout);
    //fprintf(stdout, "[collect_data_thread]: I'm  in collector thread before post(src:empty) for all\n");
    //fflush(stdout);
    for (int i = 0; i < collector->sources_cnt; i++) {
        struct data_source *src = collector->sources[i];
        strcpy(src->data, collector->data);
        sem_post(&src->empty_ready);
    }
    //fprintf(stdout, "[collect_data_thread]: I'm  in collector thread after post(src:empty) for all\n");
    //fflush(stdout);
    while (!collector->exit_flag) {
        //fprintf(stdout, "[collect_data_thread]: I'm  in collector thread before wait(src:data)\n");
        //fflush(stdout);
        for (int i = 0; i < collector->sources_cnt; i++) {
            struct data_source *src = collector->sources[i];
            //fprintf(stdout, "[collect_data_thread, src = %d]: I'm  in collector thread before wait(src:data)\n", src->fd);
            //fflush(stdout);
            sem_wait(&src->data_ready);
            //fprintf(stdout, "[collect_data_thread, src = %d]: I'm  in collector thread after wait(src:data)\n", src->fd);
            //fflush(stdout);
        }
        //TODO: only for last active source
        //fprintf(stdout, "[collect_data_thread]: I'm  in collector thread before post(collector:data)\n");
        //fflush(stdout);
        sem_post(&collector->data_ready);
        //fprintf(stdout, "[collect_data_thread]: I'm  in collector thread after post(collector:data)\n");
        //fflush(stdout);
        for (int i = 0; i < collector->sources_cnt; i++) { 
            int cnt = 0;
            struct data_source *src = collector->sources[i];
            if (src->exit_flag) {
                cnt++;
            }
            if (cnt == collector->sources_cnt) collector->exit_flag = 1;
        }
        fprintf(stdout, "[collect_data_thread]: exit_flag = %d\n", collector->exit_flag);
        fflush(stdout);
        if (!collector->exit_flag) {
            //fprintf(stdout, "[collect_data_thread]: I'm  in collector thread before wait(collector:empty)\n");
            //fflush(stdout);
            sem_wait(&collector->empty_ready);
            //fprintf(stdout, "[collect_data_thread]: I'm  in collector thread after wait(collector:empty)\n");
            //fflush(stdout);
            for (int i = 0; i < collector->sources_cnt; i++) {
                struct data_source *src = collector->sources[i];
                //fprintf(stdout, "[collect_data_thread]: Update src->data from %s to %s\n", src->data, collector->data);
                //fflush(stdout);
                strcpy(src->data, collector->data);
                //fprintf(stdout, "[collect_data_thread]: New src->data in %d is %s\n", src->fd, src->data);
                //fflush(stdout);
                //fprintf(stdout, "[collect_data_thread]: I'm  in collector thread before post(src:empty)\n");
                //fflush(stdout);
                sem_post(&src->empty_ready);
                //fprintf(stdout, "[collect_data_thread]: I'm  in collector thread after post(src:empty)\n");
                //fflush(stdout);
            }
        }
        else {
            //Stop all sources if collector stopping...
            for (int i = 0; i < collector->sources_cnt; i++) {
                struct data_source *src = collector->sources[i];
                src->exit_flag = 1;
            }
        }
    }
    //Stop all sources if collector stopping...
    for (int i = 0; i < collector->sources_cnt; i++) {
        struct data_source *src = collector->sources[i];
        src->exit_flag = 1;
        //fprintf(stdout, "[collect_data_thread]: I'm  in collector thread before wait(src:data)\n");
        //fflush(stdout);
        sem_wait(&src->data_ready);
        //fprintf(stdout, "[collect_data_thread]: I'm  in collector thread after wait(src:data)\n");
        //fflush(stdout);
    }
    //fprintf(stdout, "[collect_data_thread]: I'm  in collector thread before post(collector:data)\n");
    //fflush(stdout);
    sem_post(&collector->data_ready);
    //fprintf(stdout, "[collect_data_thread]: I'm  in collector thread after post(collector:data)\n");
    //fflush(stdout);
    fprintf(stdout, "[collect_data_thread]: stop\n");
    fflush(stdout);
    return NULL;
}

struct data_collector *collector_create() {
    struct data_collector *collector = (struct data_collector *)malloc(sizeof(struct data_collector));
    if (!collector) {
        fprintf(stderr, "[%s]: can't allocte collector\n", __FUNCTION__);
        fflush(stderr);
        return NULL;
    }
    memset(collector, 0, sizeof(struct data_collector));
    collector->thread_id = -1;

    //!!! HARDCODE: memory size !!!// -- used for collector server/clients
    collector->chunk_cnt = 512;
    collector->chunk_size = 256; //chunk_size enought for obj_name (strlen("object_")+strlen(data_postfix))
    collector->data_memory = (void*)malloc(collector->chunk_cnt * collector->chunk_size * sizeof(char));
    collector->dataoffset = 0;
    collector->data_ring = ring_buffer_create(collector->chunk_cnt, 0);

    return collector;
}

void collector_set_boost_shmem(struct data_collector *collector, int use_boost_shmem, char *boost_path, size_t boost_size) {
    if (use_boost_shmem) {
        collector->use_boost_shmem = 1;
        strcpy(collector->boost_path, boost_path);
        collector->boost_size = boost_size;
    }
}

struct data_collector *collector_init(struct collector_description *desc) {

    struct data_collector *collector = collector_create();

    if (!desc) {
        fprintf(stdout, "[collector_init]: no collector description, create default (w/o src)\n");
        fflush(stdout);
        struct collector_description *desc = NULL;
        collector_description_create(&desc);
        //TODO: this way wasn't testing -- do something bad
    }
    fprintf(stdout, "[collector_init]: check collector configuration\n");
    fflush(stdout);
    out_collector_description(desc);

    //!!! HARDCODE: memory size !!!// -- used for collector server/clients
    collector->chunk_cnt = 512;
    collector->chunk_size = 256; //chunk_size enought for obj_name (strlen("object_")+strlen(data_postfix))
    collector->data_memory = (void*)malloc(collector->chunk_cnt * collector->chunk_size * sizeof(char));
    collector->dataoffset = 0;
    collector->data_ring = ring_buffer_create(collector->chunk_cnt, 0);

    collector->sources_cnt = desc->sources_cnt;
    for (int i = 0; i < collector->sources_cnt; i++) {
        struct data_source *src = collector->sources[i];
        int fd = source_init(&src, desc->sdescs[i]);
        src->fd = fd;
        src->sid = i;
        //put place for frames to source
        strcpy(src->data, collector->data);
        //collector->sources[i].add_frame_callback = NULL; //paranoja, set in source
        collector->sources[i] = src;
        collector->exit_flag = 0;
    }

    collector_set_boost_shmem(collector, desc->use_boost_shmem, desc->boost_path, desc->boost_size); 

    if (sem_init(&(collector->data_ready), 1, 1) == -1) {
        fprintf(stderr, "[collector]: can't init object collector semaphore\n");
        return NULL;
    }

    if (sem_init(&(collector->empty_ready), 1, 0) == -1) {
        sem_destroy(&collector->data_ready);
        fprintf(stderr, "[collector]: can't init object collector semaphore\n");
        return NULL;
    }

    pthread_create(&collector->thread_id, NULL, collect_data_thread, collector);

    return collector;
}

void collector_destroy(struct data_collector *collector){
    collector->exit_flag = 1;
    if (collector->thread_id != (pthread_t)-1 ) pthread_join(collector->thread_id, NULL);
    for (int i = 0; i < collector->sources_cnt; i++) {
        source_destroy(collector->sources[i]);
    }
    sleep(5);
    sem_destroy(&collector->empty_ready);
    sem_destroy(&collector->data_ready);
    ring_buffer_destroy(collector->data_ring);
    free(collector);
}
