#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <misc/log.h>
#include <misc/name_generator.h>

#include <storage/storage_interface.h>

#include <shmem/memory_map.h>
#include <shmem/shared_memory.h>

#include <pipeline/pipeline_client.h>
#include <pipeline/pipeline_server.h>

#include <processor/dclprocessor.h>
#include <description/description_json.h>
#include <description/dclprocessor_description.h>

//first
void first_action(void *internal_data, void *data, int *status) {
    struct dclprocessor *dclproc = (struct dclprocessor*)internal_data;
    if (!dclproc) return;
    struct data_collector *collector = dclproc->collector;
    if (!collector) {
        return;
    }
    char obj_name[256];
    if (collector->exit_flag) {
        sem_post(&(collector->empty_ready));
        *status = 0;
        return;
    }

    LOG_FD_INFO("I'm in collector action before wait(collector:data)\n");
    sem_wait(&(collector->data_ready));

    static int data_id = 0; //debug information
    LOG_FD_INFO("Object #%d with name %s is ready\n", data_id++, collector->data);

#if 0 //this part for collector with self ring buffer
    //prepare place for current ready collector->data(=full_object_name) in the collector->data_ring
    data = (char*)collector->data_memory + collector->dataoffset;
    //copy string to ring memory
    strcpy((char*)data, collector->data);
    ring_buffer_push(collector->data_ring, collector->dataoffset);
    LOG_FD_INFO("Dataoffset = %zu pushed to data_ring, data = %s(%p)\n", collector->dataoffset, (char*)data, (void*)data);
    collector->dataoffset = (collector->dataoffset + collector->chunk_size) % (collector->chunk_cnt * collector->chunk_size);
#else
    strcpy((char*)data, collector->data);
    LOG_FD_INFO("Take current collector data = %s(%p)\n", (char*)collector->data, (void*)collector->data);
#endif

    //generate name for next object
    generate_new_object_name(obj_name);
    storage_interface_new_object_with_framesets(collector->boost_path, obj_name, collector->sources_cnt);
    LOG_FD_INFO("%s created\n", obj_name);

    //Apply callback to new object -- normally to do nothing
    if (dclproc->client_pipeline_action != NULL) {
        dclproc->client_pipeline_action(dclproc->user_data_ptr, collector->boost_path, obj_name);
    }

    LOG_FD_INFO("Set new generated name %s to collector->data\n", obj_name);
    strcpy(collector->data, obj_name); //active collector object for add frame
    LOG_FD_INFO("I'm  in first action before post(collector:empty)\n");
    sem_post(&(collector->empty_ready));
    LOG_FD_INFO("I'm  in first action after post(collector:empty)\n");
    *status = 0;
}

//middle -- I'm not sure that collector ring is necessary 
void middle_action(void *internal_data, void *data, int *status) {
    LOG_FD_INFO("");
    struct dclprocessor *dclproc = (struct dclprocessor*)internal_data;
    if (!dclproc) return;

    //only collector known about memory where the dataset is
    struct data_collector *collector = dclproc->collector;
    if (!collector) {
        return;
    }

#if 0 //this part for the module after collector with self ring buffer 
    //TODO: think about self buffer for any module, but it some more difficult the one ring for all pipeline
    size_t dataoffset = ring_buffer_pop(collector->data_ring); // think about take =) 
    char *obj_name = (char*)collector->data_memory + dataoffset;
    LOG_FD_INFO("%zu poped from data_ring, data = %s[%p]\n", dataoffset, obj_name, (void*)obj_name);
#else
    char *obj_name = (char *)data;
    LOG_FD_INFO("data = %s[%p]\n", obj_name, (void*)obj_name);
#endif
    if (strlen(obj_name) > 0) {
        //strcpy((char*)data, obj_name);

        //Check object before module work
        //storage_interface_analyze_object_with_framesets(collector->boost_path, obj_name);

        //Apply callback to new object -- normally to do nothing
        if (dclproc->client_pipeline_action != NULL) {
            dclproc->client_pipeline_action(dclproc->user_data_ptr, collector->boost_path, obj_name);
        }

        //Check object before module work
        //storage_interface_analyze_object_with_framesets(collector->boost_path, obj_name);

        *status = 0;
    }
    else {
        *status = 1;
    }
}

void last_action(void *internal_data, void *data, int *status) {
    LOG_FD_INFO("");
    struct dclprocessor *dclproc = (struct dclprocessor*)internal_data;

    //only collector known about memory where the dataset is
    struct data_collector *collector = dclproc->collector;
    if (!collector) {
        return;
    }

    //void *obj_ptr;
    char *obj_name = (char *)data;
    static int obj_id = 0; //debug information
    fprintf(stdout, "[last_action, #%d]: %s\n", obj_id++, obj_name);
    fflush(stdout);
    // It should be very strange if the middle module do nothing
    //Check object before module work
    //storage_interface_analyze_object_with_framesets(dclproc->boost_path, obj_name);

    //Apply callback to new object -- normally to do nothing
    if (dclproc->client_pipeline_action != NULL) {
        dclproc->client_pipeline_action(dclproc->user_data_ptr, collector->boost_path, obj_name);
    }

    //Check object before module work
    //storage_interface_analyze_object_with_framesets(collector->boost_path, obj_name);

    storage_interface_free_object(collector->boost_path, obj_name);
    *status = 0;
}

//server and common client initializations
int dclprocessor_init(struct dclprocessor **p_dclproc, struct dclprocessor_description *desc) {
    int rc;

    if (*p_dclproc && (*p_dclproc)->need_to_free) free(*p_dclproc);
    struct dclprocessor *dclproc = (struct dclprocessor *)malloc(sizeof(struct dclprocessor));
    if (!dclproc) {
        LOG_FD_ERROR("can't allocte dclprocessor\n");
        return -1;
    }

    memset(dclproc, 0, sizeof(struct dclprocessor));
    dclproc->need_to_free = 1;

    if (!desc) {
        LOG_FD_INFO("no dclprocessor description, create default\n");
        desc = dclprocessor_description_create();
        if (!desc) {
            LOG_FD_ERROR("WTF? Default descriptor is null\n");
            return -1;
        }
        dclproc->desc = desc;
    }

    LOG_FD_INFO("Check dclprocessor configuration...\n");
    out_dclprocessor_description(desc);

    struct pipeline_description *pdesc = desc->pipeline_desc;

    if (desc->use_shmem && desc->shmpath) {
        //processes in pipeline
        int proc_cnt = (pdesc) ? (pdesc->modules_cnt + 2) : 2;
        pdesc->proc_cnt = proc_cnt;
        int proc_size = pipeline_get_procinfo_size();
        //data objects in pipeline, should be more then processes_cnt + 2 (initial/finalization) + 2 (reserve)
        int data_cnt = (pdesc && pdesc->data_cnt > pdesc->proc_cnt + 2) ? pdesc->data_cnt : proc_cnt + 2;
        //minimal data size is char[42]
        int data_size = (pdesc) ? pdesc->data_size : DCL_MAX_DATA_SIZE;
        //description zones by DESCRIPTOR_DATA_MAX_LEN (16K)
        int desc_cnt = 1;
        void *map = memory_map_create(proc_cnt, proc_size, data_cnt, data_size, desc_cnt, DESCRIPTOR_DATA_MAX_LEN);
        shared_memory_unlink(desc->shmpath);
        void *shmem = shared_memory_server_init(desc->shmpath, map);
        free(map);
        if (!shmem) {
            LOG_FD_ERROR("can't init shared memory\n");
            free(dclproc);
            return -1;
        }
        dclproc->shmem = shmem;
        desc->shmsize = shared_memory_get_size(dclproc->shmem);
        if (pdesc) {
            if (pdesc->shmpath) {
                LOG_FD_INFO("Pipeline shared memory settings will discarded!\n");
                free(pdesc->shmpath);
            }
            pdesc->shmpath = strdup(desc->shmpath);
            pdesc->shmsize = desc->shmsize;
            pdesc->use_common_shmem = 1;
        }
        shared_memory_setenv(desc->shmpath, dclproc->shmem);
#if 1
        //write dclprocessor_description to the shared memory for modules
        size_t size = strlen(desc->full_json);
        LOG_FD_INFO("Full json size = %zd\n", size);
        if (dclproc->shmem) {
            shared_memory_write_descriptions_segment(dclproc->shmem, 0, (char*)&size, sizeof(size_t));
            shared_memory_write_descriptions_segment(dclproc->shmem, sizeof(size_t), desc->full_json, strlen(desc->full_json));
       }
#endif
    }
    else {
        LOG_FD_INFO("Only common shmem possible in this version of dclprocessor\n");
    }

    //out_pipeline_description(desc->pipeline_desc);
    rc = pipeline_init(&dclproc->pipeline, desc->pipeline_desc);
    if (rc != 0) {
        LOG_FD_ERROR("couldn't init pipeline\n");
        LOG_FD_INFO("Configuration without pipeline is not supported yet...\n");
        return -1;
    }

    //IMPORTANT! Pipeline shouldn't start LIVE while some processes are not initializing
    pipeline_start(dclproc->pipeline);

#if 1
    //sleep(1);
    //pipeline_pause(dclproc->pipeline);
    sleep(3);
    pipeline_live(dclproc->pipeline);
#endif

#if 0
    rc = collector_init(&dclproc->collector, desc->collector_desc);
    if (rc != 0) {
        LOG_FD_ERROR("couldn't init collector\n");
        LOG_FD_INFO("Configuration without collector is not supported yet...\n");
        return -1;
    }
#endif

    *p_dclproc = dclproc;
    return 0;
}

//up full system structure from shared memory or default
//client part initialization (by ENV when process_start() will executed)
struct dclprocessor *dclprocessor_get(int type) {
    struct dclprocessor *dclproc = (struct dclprocessor *)malloc(sizeof(struct dclprocessor));
    if (!dclproc) {
        LOG_FD_ERROR("can't allocte dclprocessor\n");
        return NULL;
    }
    memset(dclproc, 0, sizeof(struct dclprocessor));
    dclproc->need_to_free = 1;

    dclproc->type = type;

    char *shm_path = NULL;
    size_t shm_size;

    shared_memory_getenv(&shm_path, &shm_size);
    LOG_FD_INFO("Found shared memory with size = %zd at '%s'\n", shm_size, shm_path);

    if (shm_path == NULL) {
        LOG_FD_INFO("Author of this code is not smart\n");
    }

    void *shmem = shared_memory_client_init(shm_path, shm_size);
    if (shmem == NULL) {
        return NULL;
    }

    //read real description size
    size_t size = 0;
    shared_memory_read_descriptions_segment(shmem, 0, (char*)&size, sizeof(size_t));
    //dclproc->desc->full_json_size = size;
    //read description
    char *buffer = (char *)malloc(size * sizeof(char));
    shared_memory_read_descriptions_segment(shmem, sizeof(size_t), buffer, size);
    //LOG_FD_INFO("Buffer = %s\n", buffer);
    load_dclprocessor_description_from_buffer(buffer, size, &dclproc->desc);

    struct collector_description *desc = dclproc->desc->collector_desc;

    if (dclproc->type == 1 && dclproc->collector == NULL) {
        dclproc->collector = collector_init(desc);
    }
    else {
        dclproc->collector = collector_create();
        collector_set_boost_shmem(dclproc->collector, desc->use_boost_shmem, desc->boost_path, desc->boost_size);
    }

    if (!dclproc->collector) {
        LOG_FD_INFO("Dclprocessor without collector and boost shmem is not supported now\n");
    }

    return dclproc;
}

void dclprocessor_destroy(struct dclprocessor **p_dclproc) {
    if (p_dclproc == NULL || *p_dclproc == NULL)  return;
    struct dclprocessor *dclproc = *p_dclproc;
    struct dclprocessor_description *desc = dclproc->desc;
    if (desc && desc->use_shmem && desc->shmpath) {
        shared_memory_destroy(desc->shmpath, dclproc->shmem);
        dclproc->shmem = NULL;
        free(desc->shmpath);
        desc->shmpath = NULL;
    }
    if (dclproc->pipeline) pipeline_destroy(dclproc->pipeline);
    if (dclproc->collector) collector_destroy(dclproc->collector);
    //TODO: process subsystem
    if (dclproc->need_to_free) free(dclproc);
    *p_dclproc = NULL;
}

void out_dclprocessor_status(struct dclprocessor *dclproc){
    (void)dclproc;
    LOG_FD_INFO("It will be system status. Not now.\n");
}

//void dclprocessor_attach (
void attach_to_pipeline (
         struct dclprocessor *dclproc, void* user_data_ptr,
         void (*work_with_object)(void * user_data_ptr, char *boost_path, char *obj_name)
                        ) {

    if (!dclproc) {
        LOG_FD_INFO("Current module couldn't work in pipeline (pipeline env is empty)\n");
        return;
    }

    dclproc->user_data_ptr = user_data_ptr;
    dclproc->client_pipeline_action = work_with_object;

    switch (dclproc->type) {
        case 0:
            LOG_FD_INFO("type = %d\n", dclproc->type);
            pipeline_process((void *)dclproc, last_action);
            break;
        case 1:
            LOG_FD_INFO("type = %d\n", dclproc->type);
            if (dclproc->collector) {
                //TODO: set ops for dclproc/collector/sources ???
                pipeline_process((void *)dclproc, first_action);
            }
            else {
                LOG_FD_INFO("First action without collector is not supported\n");
            }
            break;
        case 2:
            LOG_FD_INFO("type = %d\n", dclproc->type);
            pipeline_process((void *)dclproc, middle_action);
            break;
        default:
            LOG_FD_INFO("type = %d\n", dclproc->type);
    }
}


