#ifndef _CV_COLLECTOR_H_
#define _CV_COLLECTOR_H_

#include <semaphore.h>
#include <pthread.h>

struct cv_data_source {
    union {
        int fd;
        char addr[12];
        char port[8];
    };
};

struct cv_data_collector {
    int exit_flag;
    pthread_t thread_id;
    struct cv_data_source src;
    char segment[80];
    sem_t empty_ready;
    sem_t data_ready;
    char data[256];
};

struct cv_data_collector *cv_collector_init(char* addr, char *port);
void cv_collector_destroy(struct cv_data_collector *collector);

#endif //_CV_COLLECTOR_H_
