#ifndef _PIPELINE_MODULE_H
#define _PIPELINE_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <semaphore.h>

#include <misc/list.h>
#include <misc/ring_buffer.h>
#include <description/pipeline_module_description.h>

enum MD_STATE {
    MD_STATE_UNKNOWN = -2,
    MD_STATE_ERROR = -1,
    MD_STATE_INIT = 0,
    MD_STATE_LIVE = 1,
    MD_STATE_PAUSE = 2,
    MD_STATE_STOP = 101
    //other values as percent of work are available -- as MD_STATE_CUSTOM
};

static inline const char *module_state_string(enum MD_STATE state) {
    switch (state) {
        case MD_STATE_UNKNOWN:
            return "MD_STATE_UNKNOWN";
        case MD_STATE_ERROR:
            return "MD_STATE_ERROR";
        case MD_STATE_INIT:
            return "MD_STATE_INIT";
        case MD_STATE_LIVE:
            return "MD_STATE_LIVE";
        case MD_STATE_PAUSE:
            return "MD_STATE_PAUSE";
        case MD_STATE_STOP:
            return "MD_STATE_STOP";
        default: //TODO: use as current percent stage
            return "MD_STATE_CUSTOM 50\%";
    }
    return "";
}

enum MD_MODE {
    MD_MODE_DETACHED = -1,
    MD_MODE_ATTACHED = 0
};

struct pipeline_module {

    list_t entry; //module is element of modules list in pipeline

    int id;  //id from description for identify module for users
    enum MD_MODE mode; //?? for control attached/detached mode
    struct pipeline_module_info *minfo;

    void *shmem;
    int idx; //idx of process in the process list, -1 for wrong process name
    struct process_info *proc_info;

    struct ring_buffer *data_ring;
    size_t dataoffset;   //offset to data from start of shared memory for the next action

    int *exit_flag;
    pthread_t thread_id;
    void *(*thread_method)(void*); //source[first], processor[middle] or destination[last]
    sem_t *start_control;

    sem_t new_cmd;
    int cmd;             //next action for module
    sem_t result;
    int last_cmd_result;           //resulting state of last executed action, unused now (?)

    struct pipeline_module *next; //for  sending commands to next module

    enum MD_STATE state; //state of module in pipeline, information for pipeline thread

    int need_to_free;
};

//as element of modules_list
struct pipeline_module *pipeline_module_get(int id, list_t *modules_list);
struct pipeline_module *pipeline_module_add(list_t *modules_list, int *modules_cnt, 
                                            struct pipeline_module_info *minfo);
void pipeline_module_delete(list_t *modules_list, int *modules_cnt, struct pipeline_module *module);
int pipeline_modules_update(list_t *modules_list);

//as part of pipeline
int pipeline_module_init(struct pipeline_module *module, 
                            void *shmem, struct ring_buffer *data_ring, 
                            int *exit_flag, sem_t *start_control);
void pipeline_module_destroy(struct pipeline_module *module);

//state machine
enum MD_STATE pipeline_module_set_state(enum MD_STATE state, struct pipeline_module *module); //returns old_state
void pipeline_modules_set_state(enum MD_STATE state, list_t *modules_list);

//for debug
void pipeline_module_debug_out(struct pipeline_module *module);
void pipeline_modules_list_debug_out(list_t *modules_list);

#ifdef __cplusplus
}
#endif

#endif //_PIPELINE_H
