#ifndef _PIPELINE_H
#define _PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <misc/list.h>
#include <description/pipeline_description.h>

enum PL_STATE {
    PL_STATE_UNKNOWN = -2,
    PL_STATE_ERROR = -1,
    PL_STATE_INIT = 0,
    PL_STATE_LIVE = 1,
    PL_STATE_PAUSE = 2,
    PL_STATE_STOP = 101
};

static inline const char *pipeline_state_string(enum PL_STATE state) {
    switch (state) {
        case PL_STATE_UNKNOWN:
            return "PL_STATE_UNKNOWN";
        case PL_STATE_ERROR:
            return "PL_STATE_ERROR";
        case PL_STATE_INIT:
            return "PL_STATE_INIT";
        case PL_STATE_LIVE:
            return "PL_STATE_LIVE";
        case PL_STATE_PAUSE:
            return "PL_STATE_PAUSE";
        case PL_STATE_STOP:
            return "PL_STATE_STOP";
        default: 
            return "NOT_DEFINED";
    }
    return "";
}

struct pipeline {
    //TODO: think about this -- not necessary for pipeline work, put all this information in the shmem
    struct pipeline_description *pdesc;

    size_t desc_size;
    size_t desc_offset;
    char *desc_buffer;

    //main pipeline thread
    pthread_t thread_id;
    int exit_flag;

    //for process_info description
    int proc_cnt; //at least 1 (real: modules_cnt + 1)
    char *shmpath; // should start with '/'
    void *shmem;

    //ring buffer parameters for store active data between processes
    int data_cnt; //number of data chunks
    int data_size; //size of each data chunk

    int modules_cnt;
    list_t modules_list; //list of pipeline_module structures

    enum PL_STATE state;

    int need_to_free;
};

size_t pipeline_get_procinfo_size();

int pipeline_init(struct pipeline **p_pl, struct pipeline_description *pdesc);
//int pipeline_init(struct pipeline **p_pl, struct pipeline_info *pinfo);
void pipeline_destroy(struct pipeline *pl);

/*
// R/W pipeline description to the special region of base shmem
size_t pipeline_write_description(struct pipeline *pl);
size_t pipeline_read_description(struct pipeline *pl);
void pipeline_check_rw_to_shmem(struct pipeline *pl);
*/

//TODO: implement this commands
int pipeline_start(struct pipeline *pl);
int pipeline_live(struct pipeline *pl);
int pipeline_pause(struct pipeline *pl);
int pipeline_stop(struct pipeline *pl);

#ifdef __cplusplus
}
#endif

#endif //_PIPELINE_H
