#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <shmem/shared_memory.h>
#include <shmem/memory_map.h>

#include <process/process_mngr.h>

#include <misc/log.h>
#include <misc/ring_buffer.h>

#include <pipeline/pipeline_module.h>
#include <pipeline/pipeline.h>

#include <description/pipeline_description.h>

size_t pipeline_get_procinfo_size() {
    return PROCESS_INFO_SIZE;
}

void* pipeline_thread(void *arg) {
    struct pipeline *pl = (struct pipeline*)(arg);
    void *shmem = pl->shmem;
    int i;

    struct memory_map *map = (struct memory_map *)(shmem);

    struct process_info *proc_info = (struct process_info *)(shmem + memory_get_process_offset(shmem));
    process_subsystem_init(proc_info, map->process_cnt);

    struct ring_buffer *data_ring = ring_buffer_create(map->data_cnt, 0);
    for (i = 0; i < map->data_cnt; i++)
        data_ring->buffer[i] = memory_get_data_offset(shmem) + i * map->data_size;

    //Kat's paranoya: check that pipeline is correct 
    if (pl->modules_cnt < 2) {
        LOG_FD_ERROR("very strange -- initial and finalization thread should be always\n");
    }

    list_t *elem = NULL;
    sem_t start_control;
    sem_init(&start_control, 0, 0);

    pl->state = PL_STATE_UNKNOWN;

    //initialize modules (pipeline part)
    int real_started = 0;
    elem = list_first_elem(&pl->modules_list);
    while (list_is_valid_elem(&pl->modules_list, elem)) {
        struct pipeline_module *module = list_entry(elem, struct pipeline_module, entry);
        LOG_FD_INFO("module->exit_flag = %p, exit_flag = %d[%p]\n", module->exit_flag, pl->exit_flag, &pl->exit_flag);
        pipeline_module_init(module, shmem, data_ring, &pl->exit_flag, &start_control);
        pthread_create(&(module->thread_id), NULL, module->thread_method, module);
        LOG_FD_INFO("module %lx on method %p waitting start\n", module->thread_id, module->thread_method);
        sem_wait(module->start_control);
        LOG_FD_INFO("module with id = %d, idx = %d started\n", module->id, module->idx);
        if (module->idx == -1) {
            //TODO: check module state
            pthread_join(module->thread_id, NULL);
            pipeline_module_set_state(MD_STATE_ERROR, module);
            pl->state = PL_STATE_ERROR;
            break;
        }
        pipeline_module_set_state(MD_STATE_INIT, module);
        real_started++;
        elem = elem->next;
    }
    sem_destroy(&start_control);
    if (real_started >= 2) {
        pl->state = PL_STATE_INIT;
    }
    else {
        pl->state = PL_STATE_ERROR;
    }

    pipeline_modules_list_debug_out(&pl->modules_list);

    //move it to pipeline_start
    int wait_on_start = pl->pdesc->wait_on_start;
    if (wait_on_start > 0) {
        LOG_FD_INFO("please, wait %ds before pipeline started 2\n", wait_on_start);
        sleep(wait_on_start);
    }

    //when pipeline stopped we should free memory
    elem = list_first_elem(&pl->modules_list);
    while (list_is_valid_elem(&pl->modules_list, elem)) {
        struct pipeline_module *module = list_entry(elem, struct pipeline_module, entry);
        if (module->idx != -1) pthread_join(module->thread_id, NULL); //else thread already joined 
        pipeline_module_destroy(module);
        elem = elem->next;
    }

    ring_buffer_destroy(data_ring);
    process_subsystem_destroy();
    return NULL;
}

int pipeline_init(struct pipeline **p_pl, struct pipeline_description *pdesc) {
    int rc = 0; //returned value = ok

    if (!pdesc) { 
        // TODO: create empty pipeline as default for add new modules "on fly"
        // not implemented now: init/start/stop/destroy in this case
        LOG_FD_ERROR("can't init pipeline without any description (\"on fly\" not supported yet)\n");
        return -1;
    }

    struct pipeline *pl = *p_pl;
    if (!pl) {
        pl = (struct pipeline *)malloc(sizeof(struct pipeline));
        memset(pl, 0, sizeof(struct pipeline));
        pl->need_to_free = 1;
    }
    if (!pl) {
        LOG_FD_ERROR("can't allocate memory for pipeline\n");
        return -1;
    }

    //only link to external pdesc
    pl->pdesc = pdesc;

    pl->state = PL_STATE_UNKNOWN;
    pl->exit_flag = 0;
    //LOG_FD_INFO("pdesc->proc_cnt = %d\n", pdesc->proc_cnt);
    //+ 2 = initial and finalization threads are possible
    pl->proc_cnt = (pdesc->proc_cnt > 0) ? (pdesc->proc_cnt + 2) : PROCESS_MAX_CNT;
    pl->data_cnt = pdesc->data_cnt;
    pl->data_size = pdesc->data_size;
    pl->shmpath = strdup(pdesc->shmpath);

    if (!pdesc->use_common_shmem) {
        void *map = memory_map_create(pl->proc_cnt, PROCESS_INFO_SIZE, pl->data_cnt, pl->data_size, 1, DESCRIPTOR_DATA_MAX_LEN);
        shared_memory_unlink(pl->shmpath);
        void *shmem = shared_memory_server_init(pl->shmpath, map);
        free(map);
        if (!shmem) {
            LOG_FD_ERROR("can't init shared memory\n");
            *p_pl = NULL;
            return -1;
        }
        pl->shmem = shmem;
        shared_memory_setenv(pl->shmpath, pl->shmem);
    }
    else {
        pl->shmem = shared_memory_client_init(pdesc->shmpath, pdesc->shmsize);
    }

    if (!pl->shmem) {
        LOG_FD_ERROR("pipeline memory wasn't init\n");
        *p_pl = NULL;
        return -1;
    }

    LOG_FD_INFO("pdesc->modules_cnt = %d\n", pdesc->modules_cnt);

    list_init(&pl->modules_list);
    for (int i = 0; i < pdesc->modules_cnt; i++) {
        pipeline_module_add(&pl->modules_list, &pl->modules_cnt, pdesc->minfos[i]);
    }
    rc = pipeline_modules_update(&pl->modules_list);
    if (rc != 0) {
        LOG_FD_ERROR("problems with modules configuration\n");
        pipeline_destroy(pl);
        *p_pl = NULL;
        return -1;
    }

    LOG_FD_INFO("added %d modules to pipeline\n", pl->modules_cnt);

    rc = pthread_create(&pl->thread_id, NULL, pipeline_thread, pl);
    if (rc != 0) {
        LOG_FD_INFO("can't create pthread, check pipeline structure\n");
    }

    *p_pl = pl;
    return 0;
}

/*
void pipeline_check_rw_to_shmem(struct pipeline *pl) {
    char *buffer = NULL;
    size_t size = 0;
    LOG_FD_INFO("prepare pl->desc_buffer\n");
    pipeline_write_description(pl);
    buffer = strdup(pl->desc_buffer);
    size = pl->desc_size;
#if 1
    LOG_FD_INFO("writting %zd bytes to the shmem\n", size);
    LOG_FD_INFO("----- Begin buffer -----\n");
    LOG_FD_INFO("(%zd bytes) %s\n", size, buffer);
    LOG_FD_INFO("----- End buffer -----\n");
#endif
    pipeline_read_description(pl);
#if 1
    LOG_FD_INFO("reading %zd bytes of pl->desc_buffer to the shmem\n", size);
    LOG_FD_INFO("----- Begin buffer -----\n");
    LOG_FD_INFO("(%zd bytes) %s\n", size, buffer);
    LOG_FD_INFO("----- End buffer -----\n");
#endif
    int rc = 0;
    if (size != pl->desc_size || (rc = memcmp(buffer, pl->desc_buffer, size)) != 0) {
        LOG_FD_ERROR("R/W to shmem is failed, size = %zd, pl->desc_size = %zd, rc = %d\n", size, pl->desc_size, rc);
    }
    else {
        LOG_FD_INFO("R/W to shmem is Ok\n"); 
    }
}

//We suggest here that shmem is pointer to start of full available region (with the map in the begin)
size_t pipeline_write_description(struct pipeline *pl) {
    if (!pl || !pl->pdesc || !pl->shmem) { 
        LOG_FD_ERROR("no pipeline, description or shmem not correct\n");
        return 0;
    }

    char *buffer = NULL;
    size_t size = write_pipeline_description_to_buffer(&buffer, pl->pdesc);
    shared_memory_write_descriptions_segment(pl->shmem, 0, buffer, size); //memcpy

    LOG_FD_INFO("writting %zd bytes to the shmem\n", size);
#if 0
    LOG_FD_INFO("----- Begin buffer -----\n");
    LOG_FD_INFO("(%zd bytes) %s\n", size, buffer);
    LOG_FD_INFO("----- End buffer -----\n");
#endif

    //not necessary
    if (pl->desc_buffer) {
        LOG_FD_INFO("ATTENTION! pl->desc_buffer will be rewrited\n");
        free(pl->desc_buffer);
    }
    pl->desc_size = size;
    pl->desc_buffer = strdup(buffer);
    
    free(buffer);
    return size;
}

size_t pipeline_read_description(struct pipeline *pl) {
    if (!pl || !pl->pdesc || !pl->shmem) { 
        LOG_FD_ERROR("no pipeline, description or shmem not correct\n");
        return 0;
    }

    //TODO: hardcode, use realloc or make other implementation
    size_t size = DESCRIPTOR_DATA_MAX_LEN;
    char buffer[DESCRIPTOR_DATA_MAX_LEN];
    shared_memory_read_descriptions_segment(pl->shmem, 0, buffer, size); //in this place memcpy

    if (pl->pdesc) {
        LOG_FD_INFO("ATTENTION! pl->pdesc will be rewrited\n");
        pipeline_description_destroy(pl->pdesc);
        pl->pdesc = NULL;
    }
    read_pipeline_description_from_buffer(buffer, &pl->pdesc);
    size = strlen(buffer);

    LOG_FD_INFO("reading %zd bytes of pl->desc_buffer to the shmem\n", size);
#if 0
    LOG_FD_INFO("----- Begin buffer -----\n");
    LOG_FD_INFO("(%zd bytes) %s\n", size, buffer);
    LOG_FD_INFO("----- End buffer -----\n");
#endif

    //not necessary
    if (pl->desc_buffer) {
        LOG_FD_INFO("ATTENTION! pl->desc_buffer will be rewrited\n");
        free(pl->desc_buffer);
    }
    pl->desc_size = size;
    pl->desc_buffer = strdup(buffer);
    return size;
}
*/

int pipeline_start(struct pipeline *pl) {
    if (!pl) return -1;
    while (pl->state != PL_STATE_INIT) {
        usleep(100);
    }
    if (pl->state == PL_STATE_INIT) {
        pipeline_modules_set_state(MD_STATE_PAUSE, &pl->modules_list);
        pl->state = PL_STATE_PAUSE;
    }
    //pipeline_live(pl);
    return 0;
}

int pipeline_live(struct pipeline *pl) {
    if (!pl) return -1;
    if (pl->state == PL_STATE_PAUSE) {
        pipeline_modules_set_state(MD_STATE_LIVE, &pl->modules_list);
        pl->state = PL_STATE_LIVE;
    }
    else {
        LOG_FD_INFO("unsupported, pl->state = %s is not PAUSE\n", pipeline_state_string(pl->state));
    }
    return 0;
}

int pipeline_pause(struct pipeline *pl) {
    if (!pl) return -1;
    if (pl->state == PL_STATE_LIVE) {
        pipeline_modules_set_state(MD_STATE_PAUSE, &pl->modules_list);
        pl->state = PL_STATE_PAUSE;
    }
    else {
        LOG_FD_INFO("unsupported, pl->state = %s is not LIVE\n", pipeline_state_string(pl->state));
    }
    return 0;
}

int pipeline_stop(struct pipeline *pl) {
    if (!pl) return -1;
    LOG_FD_INFO("not fully implementation\n");
    pipeline_modules_set_state(MD_STATE_STOP, &pl->modules_list);
    pl->state = PL_STATE_STOP;
    return 0;
}

void pipeline_destroy(struct pipeline *pl) {
    struct pipeline_module *module;

    pthread_join(pl->thread_id, NULL);
    LOG_FD_INFO("main pipeline thread 0x%lx finished\n", pl->thread_id);

    pipeline_modules_set_state(MD_STATE_UNKNOWN, &pl->modules_list);
    pl->state = PL_STATE_UNKNOWN;
    pl->exit_flag = 1;

    if (!pl->pdesc || (pl->pdesc && !pl->pdesc->use_common_shmem)) {
        shared_memory_destroy(pl->shmpath, pl->shmem);
        free(pl->shmpath);
    }

    //pdesc is pointer to external pdesc
    pl->pdesc = NULL;

    //delete modules from list
    list_t *item;
    while(!list_is_empty(&pl->modules_list)) {
        item = list_first_elem(&pl->modules_list);
        module = list_entry(item, struct pipeline_module, entry);
        pipeline_module_delete(&pl->modules_list, &pl->modules_cnt, module);
        pipeline_module_destroy(module);
    }

    if (pl->desc_buffer) free(pl->desc_buffer);

    if (pl->need_to_free) free(pl);
}

