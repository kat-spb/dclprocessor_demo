#ifndef _GIGE_CAMERA_H_
#define _GIGE_CAMERA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <pthread.h>
#include "udpcapture/device.h"
#include "udpcapture/iframe.h"
#include "udpcapture/gige_camera_ops.h"

enum gige_camera_state {
    STREAM_NOT_INITIALIZED = -1,
    STREAM_STOP = 0,
    STREAM_LIVE = 1,
    STREAM_PAUSE
};

struct gige_camera {
    struct device *dev;
    enum gige_camera_state state;
    struct gige_camera_ops *ops;

    int gvcp_fd;
    int gvsp_fd;
    //pthread_t control_thread_id; -- not neccessary: we sure that any camera will be started in the new source thread
    pthread_t data_thread_id;

    uint32_t last_gvcp_packet_id;

    uint32_t n_received_packets;
    uint32_t n_error_packets;
    uint32_t n_ignored_packets;

    uint64_t timestamp_tick_frequency;
    uint32_t packet_size;

    int extended_ids;
    uint32_t last_frame_id;

    void *parent;
    struct iframe_data iframe;
    void (*new_frame_action)(void *parent, void *iframe);

};

void* camera_gvsp_thread(void *arg);  //arg is struct gige_camera

//TODO: struct gige_camera *camera_init(char *sid_string);
struct gige_camera *gige_camera_init(struct device *dev, void (*new_frame_action)(void *, void *));
void gige_camera_destroy(struct gige_camera *cam);

//set duration == 0 for stream always
int gige_camera_test(struct gige_camera *cam, int duration);
//int gige_camera_start(struct gige_camera *cam);
//int gige_camera_stop(struct gige_camera *cam);

#ifdef __cplusplus
}
#endif

#endif //_GIGE_CAMERA_H_

