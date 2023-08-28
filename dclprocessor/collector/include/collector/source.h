#ifndef _SOURCE_H_
#define _SOURCE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <semaphore.h>
#include <pthread.h>
#include <description/source_description.h>

#define OBJECT_DETECTION_FRAME_LIMIT 50

struct data_packet {
    char *header;
    size_t header_len;
    char *data;
    size_t data_len;
};

struct data_source {
    int sid;
    int fd;
    struct source_description *sdesc;

    struct data_packet *packet;
    struct gige_camera *camera;

    int exit_flag;
    pthread_t thread_id;
    sem_t empty_ready;
    sem_t data_ready;
    char data[256];

    void *internal_data; //struct dclprocessor 
    int (*object_detection_callback) (void *internal_data, int source_id, int frame_id, void *frame_data);
    void (*add_frame_callback) (void *internal_data, int source_id, int frame_id, void *frame_data);
    void (*save_frame_callback) (void *internal_data, int source_id, int frame_id, void *frame_data);

    int need_to_free;
    //tmp: think about better control
    int frames_in_object;
    int object_detection_frame_limit;
};

int source_init(struct data_source **psrc, struct source_description *sdesc);
void source_destroy(struct data_source *source);

#ifdef __cplusplus
}
#endif

#endif //_SOURCE_H_
