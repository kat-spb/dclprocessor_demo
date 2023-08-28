#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <misc/log.h>
#include <misc/ring_buffer.h>

#include <process/process_mngr.h>

#include <shmem/shared_memory.h>
#include <shmem/memory_map.h>

#include <pipeline/pipeline_module.h>
#include <pipeline/pipeline.h>

//finalization thread return place in shared memory for data 
void *finalization_thread(void *arg){
    struct pipeline_module *module = arg;
    size_t dataoffset;
    char *data;
    int cmd;

    module->idx = process_start(NULL);
    module->proc_info = (struct process_info *)((char*)module->shmem + memory_get_process_offset(module->shmem)) + module->idx;

    sem_post(module->start_control);
    //not by exit flag because finalization should be always
    while(1){
        //fprintf(stdout, "[%s, %lx]: I'm here, state %s\n", __FUNCTION__, module->thread_id, state_string(module->state));
        //fflush(stdout); 
        //fprintf(stdout, "[%s, %lx, %s]: before WAIT module->new_cmd\n", __FUNCTION__, module->thread_id, module_state_string(module->state));
        //fflush(stdout);
        sem_wait(&module->new_cmd);
        //fprintf(stdout, "[%s, %lx, %s]: after WAIT module->new_cmd\n", __FUNCTION__, module->thread_id, module_state_string(module->state));
        //fflush(stdout);
        cmd = module->cmd; //TODO: queue???
        dataoffset = module->dataoffset;
        //fprintf(stdout, "[%s, %lx, %s]: before POST module->result\n", __FUNCTION__, module->thread_id, module_state_string(module->state));
        //fflush(stdout);
        sem_post(&module->result);
        //fprintf(stdout, "[%s, %lx, %s]: after POST module->result\n", __FUNCTION__, module->thread_id, module_state_string(module->state));
        //fflush(stdout);
        if (cmd == PROCESS_CMD_STOP){
            // previous process stop their job, finish pipeline
            //fprintf(stdout, "[%s, %lx, %s]: previous process stop their job, finish pipeline\n", __FUNCTION__, module->thread_id, module_state_string(module->state));
            //fflush(stdout); 
            break;
        }
        data = (char*)module->shmem + dataoffset;
        //fprintf(stdout, "[%s, %lx, %s]: clean data %p, data: %s\n", __FUNCTION__, module->thread_id, module_state_string(module->state), data, data);
        //fflush(stdout);
        data[0] = '\0';
        // no child process, only thread
        ring_buffer_push(module->data_ring, dataoffset);
    }
    LOG_FD_INFO("[%lx, %s]: stop\n", module->thread_id, module_state_string(module->state));
    //sem_post(&module->proc_info->sem_result);
    pipeline_module_set_state(MD_STATE_INIT, module);
    return NULL;
}

//initial thread take place in shared memory for data 
//TODO: implement sense implementation of initial thread, we can use this one as watchdog
void *initial_thread(void *arg){
    struct pipeline_module *module = arg;
    size_t dataoffset;
    char *data;
    int cmd;

    module->idx = process_start(NULL);
    module->proc_info = (struct process_info *)((char*)module->shmem + memory_get_process_offset(module->shmem)) + module->idx;

    sem_post(module->start_control);

    while(!(*(module->exit_flag))){
        if (module->state == MD_STATE_STOP) {
            LOG_FD_INFO("Pipeline was stopped, exit_flag = %d\n", *module->exit_flag);
            break;
        }
        if (module->state != MD_STATE_LIVE) {
            LOG_FD_INFO("[%lx]: I'm here, state %s\n", module->thread_id, module_state_string(module->state));
            fflush(stdout); 
            continue;
        }
        cmd = module->cmd; //TODO: queue
        dataoffset = ring_buffer_pop(module->data_ring);
        data = (char*)module->shmem + dataoffset;
        strcpy(data, "test");
        //data[0] = '\0';
        LOG_FD_INFO("[%lx, %s]: initial data capture %p, data %s\n", module->thread_id, module_state_string(module->state), data, data);
        LOG_FD_INFO("[%lx, %s]: next module is external process\n", module->thread_id,  module_state_string(module->state));
        module->proc_info->cmd = cmd;
        module->proc_info->cmd_offs = dataoffset;
        //if (module->next->idx > 1) { //case when (module->next) is external process
            //fprintf(stdout, "[%s, %lx, %s]: before POST module->proc_info->sem_job\n", __FUNCTION__, module->thread_id,  module_state_string(module->state));
            //fflush(stdout);
            //sem_post(&module->proc_info->sem_job);
            //fprintf(stdout, "[%s, %lx, %s]: after POST module->proc_info->sem_job\n", __FUNCTION__, module->thread_id,  module_state_string(module->state));
            //fflush(stdout);
            //fprintf(stdout, "[%s, %lx, %s]: before WAIT module->proc_info->sem_result\n", __FUNCTION__, module->thread_id,  module_state_string(module->state));
            //fflush(stdout);
            //sem_wait(&module->proc_info->sem_result);
            //fprintf(stdout, "[%s, %lx, %s]: after WAIT module->proc_info->sem_result\n", __FUNCTION__, module->thread_id,  module_state_string(module->state));
            //fflush(stdout);
        //}
        //else {
        //    fprintf(stdout, "[%s, %lx, %s]: next module is internal thread\n", __FUNCTION__, module->thread_id,  state_string(module->state));
        //    fflush(stdout);
        //}
        fprintf(stdout, "[%s, %lx, %s]: initial data captured %p, data %s\n", __FUNCTION__, module->thread_id, module_state_string(module->state), data, data);
        fflush(stdout);
        fprintf(stdout, "[%s, %lx, %s]: before WAIT module->next->result\n", __FUNCTION__, module->thread_id,  module_state_string(module->state));
        fflush(stdout);
        sem_wait(&module->next->result);
        fprintf(stdout, "[%s, %lx, %s]: after WAIT module->next->result\n", __FUNCTION__, module->thread_id,  module_state_string(module->state));
        fflush(stdout);
        module->next->cmd = PROCESS_CMD_JOB;
        module->next->dataoffset = dataoffset;
        fprintf(stdout, "[%s, %lx, %s]: next_module_idx = %d\n", __FUNCTION__, module->thread_id, module_state_string(module->state), module->next->idx);
        fflush(stdout);
        fprintf(stdout, "[%s, %lx, %s]: before POST module->next->new_cmd\n", __FUNCTION__, module->thread_id,  module_state_string(module->state));
        fflush(stdout);
        sem_post(&module->next->new_cmd);
        fprintf(stdout, "[%s, %lx, %s]: before POST module->next->new_cmd\n", __FUNCTION__, module->thread_id,  module_state_string(module->state));
        fflush(stdout);
    }

    fprintf(stdout, "[%s, %lx, %s]: stop\n", __FUNCTION__, module->thread_id, module_state_string(module->state));
    fflush(stdout);
    sem_wait(&module->next->result);
    module->next->cmd = PROCESS_CMD_STOP;
    sem_post(&module->next->new_cmd);

    if (module->next->idx > 1 ) { //next is external process
        fprintf(stdout, "[%s, %lx, %s]: next module is external process\n", __FUNCTION__, module->thread_id,  module_state_string(module->state));
        fflush(stdout);
        module->proc_info->cmd = PROCESS_CMD_STOP;
        sem_post(&module->proc_info->sem_job);
        sem_wait(&module->proc_info->sem_result);
    }
    else {
        fprintf(stdout, "[%s, %lx, %s]: next module is internal thread\n", __FUNCTION__, module->thread_id,  module_state_string(module->state));
        fflush(stdout);
    }

    pipeline_module_set_state(MD_STATE_STOP, module);

    return NULL;
}

//source thread take place in shared memory and fill it with client action
void *source_thread(void *arg){
    struct pipeline_module *module = arg;
    struct pipeline_module_info *minfo = module->minfo;
    size_t dataoffset;
    char *data;
    int cmd;

    if (!minfo) {
        LOG_FD_ERROR("WTF?!\n");
        return NULL;
    }

    if (!minfo->argv) {
        LOG_FD_ERROR("very strange first process, check configuration, please\n");
        return NULL;
    }

    module->idx = process_start(minfo->argv);
    if (module->idx < 0) {
        fprintf(stderr, "[%s, %lx, %s]: error on process %s start\n", __FUNCTION__, module->thread_id, minfo->argv[0], module_state_string(module->state));
        fflush(stderr);
        *(module->exit_flag) = 1;
        sem_post(module->start_control);
        return NULL;
    }

    module->proc_info = (struct process_info *)((char*)module->shmem + memory_get_process_offset(module->shmem)) + module->idx;
    fprintf(stdout, "[%s, %lx, %s]: %s is started\n", __FUNCTION__, module->thread_id, module_state_string(module->state), module->proc_info->process_name);
    fflush(stdout);

    enum MD_STATE old_state = module->state;

    sem_post(module->start_control);
    while(!(*(module->exit_flag))){
        if (module->state != MD_STATE_LIVE) {
            if (module->state != old_state) {
                LOG_FD_INFO("[%lx]: I'm here, state %s\n", module->thread_id, module_state_string(module->state));
                old_state = module->state;
            }
            usleep(100);
            continue;
        }
        cmd = module->cmd; //TODO: queue
        dataoffset = ring_buffer_pop(module->data_ring);
        data = (char*)module->shmem + dataoffset;
        fprintf(stdout, "[%s, %lx, %s]: capture data %p, data %s\n", __FUNCTION__, module->thread_id, module_state_string(module->state), data, data);
        fflush(stdout);
        module->proc_info->cmd = cmd;
        module->proc_info->cmd_offs = dataoffset;
        //fprintf(stdout, "[%s, %lx, %s]: before POST module->proc_info->sem_job\n", __FUNCTION__, module->thread_id,  module_state_string(module->state));
        //fflush(stdout);
        sem_post(&module->proc_info->sem_job); //before client action
        //fprintf(stdout, "[%s, %lx, %s]: after POST module->proc_info->sem_job\n", __FUNCTION__, module->thread_id,  module_state_string(module->state));
        //fflush(stdout);
        //fprintf(stdout, "[%s, %lx, %s]: before WAIT module->proc_info->sem_result\n", __FUNCTION__, module->thread_id,  module_state_string(module->state));
        //fflush(stdout);
        sem_wait(&module->proc_info->sem_result); //after client action
        //fprintf(stdout, "[%s, %lx, %s]: after WAIT module->proc_info->sem_result\n", __FUNCTION__, module->thread_id,  module_state_string(module->state));
        //fflush(stdout);
        fprintf(stdout, "[%s, %lx, %s]: data captured %p, data %s\n", __FUNCTION__, module->thread_id, module_state_string(module->state), data, data);
        fflush(stdout);
        //fprintf(stdout, "[%s, %lx, %s]: before WAIT module->next->result\n", __FUNCTION__, module->thread_id,  module_state_string(module->state));
        //fflush(stdout);
        sem_wait(&module->next->result); //next module previous result
        //fprintf(stdout, "[%s, %lx, %s]: after WAIT module->next->result\n", __FUNCTION__, module->thread_id,  module_state_string(module->state));
        //fflush(stdout);
        module->next->cmd = PROCESS_CMD_JOB;
        module->next->dataoffset = dataoffset;
        //fprintf(stdout, "[%s, %lx, %s]: before POST module->next->new_cmd\n", __FUNCTION__, module->thread_id,  module_state_string(module->state));
        //fflush(stdout);
        sem_post(&module->next->new_cmd);
        //fprintf(stdout, "[%s, %lx, %s]: before POST module->next->new_cmd\n", __FUNCTION__, module->thread_id,  module_state_string(module->state));
        //fflush(stdout);
    }

    fprintf(stdout, "[%s, %lx, %s]: stop\n", __FUNCTION__, module->thread_id, module_state_string(module->state));
    fflush(stdout);
    sem_wait(&module->next->result);
    module->next->cmd = PROCESS_CMD_STOP;
    sem_post(&module->next->new_cmd);

    module->proc_info->cmd = PROCESS_CMD_STOP;
    sem_post(&module->proc_info->sem_job);
    sem_wait(&module->proc_info->sem_result);

    pipeline_module_set_state(MD_STATE_INIT, module);

    return NULL;
}

void *process_thread(void *arg){
    struct pipeline_module *module = arg;
    struct pipeline_module_info *minfo = module->minfo;
    size_t dataoffset;
    char *data;
    int cmd;

    if (!minfo->argv) {
        LOG_FD_INFO("very strange middle process, check configuration, please\n");
        return NULL;
    }

    module->idx = process_start(minfo->argv);
    if (module->idx < 0) {
        fprintf(stderr, "[%s, %lx, %s]: error on process %s start\n", __FUNCTION__, module->thread_id, minfo->argv[0], module_state_string(module->state));
        fflush(stderr);
        *(module->exit_flag) = 1;
        sem_post(module->start_control);
        return NULL;
    }

    module->proc_info = (struct process_info *)((char*)module->shmem + memory_get_process_offset(module->shmem)) + module->idx;
    fprintf(stdout, "[%s, %lx, %s]: %s is started\n", __FUNCTION__, module->thread_id, module_state_string(module->state), module->proc_info->process_name);
    fflush(stdout);

    sem_post(module->start_control);
    //exit_flag is not correct way for stop this type of thread
    //data in the middle should visit all active steps of pipeline
    while (1) {
        fprintf(stdout, "[%s, %lx]: I'm here, state %s\n", __FUNCTION__, module->thread_id, module_state_string(module->state));
        fflush(stdout); 
        sem_wait(&module->new_cmd);
        cmd = module->cmd; //TODO: queue???
        dataoffset = module->dataoffset;
        sem_post(&module->result);
        if (cmd == PROCESS_CMD_STOP){
            // previous process stop their job, finish pipeline
            break;
        }
        data = (char*)module->shmem + dataoffset;
        fprintf(stdout, "[%s, %lx, %s]: current data %p, data: %s\n", minfo->argv[0], module->thread_id, module_state_string(module->state), data, data);
        fflush(stdout);

        module->proc_info->cmd = cmd;
        module->proc_info->cmd_offs = dataoffset;
        //fprintf(stdout, "[%s, %lx, %s]: before POST module->proc_info->sem_job\n", __FUNCTION__, module->thread_id,  state_string(module->state));
        //fflush(stdout);
        sem_post(&module->proc_info->sem_job);
        //fprintf(stdout, "[%s, %lx, %s]: after POST module->proc_info->sem_job\n", __FUNCTION__, module->thread_id,  state_string(module->state));
        //fflush(stdout);
        //fprintf(stdout, "[%s, %lx, %s]: before WAIT module->proc_info->sem_result\n", __FUNCTION__, module->thread_id,  state_string(module->state));
        //fflush(stdout);
        sem_wait(&module->proc_info->sem_result);
        //fprintf(stdout, "[%s, %lx, %s]: after WAIT module->proc_info->sem_result\n", __FUNCTION__, module->thread_id,  state_string(module->state));
        //fflush(stdout);
        if (module->proc_info->cmd_result != 0){ //TODO: think about solution for this case
            // previous process report an error, drop frame and continue
            fprintf(stdout, "[%s, %lx, %s]: processor status %d, skip data %p\n", minfo->argv[0], module->thread_id, module_state_string(module->state), module->proc_info->cmd_result, data);
            fflush(stdout);
            // pushfree buffer
            ring_buffer_push(module->data_ring, dataoffset); 
            continue;
        }
        fprintf(stdout, "[%s, %lx, %s]: processed data %p, data: %s\n", minfo->argv[0], module->thread_id, module_state_string(module->state), data, data);
        fflush(stdout);
        sem_wait(&module->next->result);
        module->next->cmd = PROCESS_CMD_JOB;
        module->next->dataoffset = dataoffset;
        sem_post(&module->next->new_cmd);
    }
    fprintf(stdout, "[%s, %lx, %s]: stop\n", minfo->argv[0], module->thread_id, module_state_string(module->state));
    fflush(stdout);
    sem_wait(&module->next->result);
    module->next->cmd = PROCESS_CMD_STOP;
    sem_post(&module->next->new_cmd);

    module->proc_info->cmd = PROCESS_CMD_STOP;
    sem_post(&module->proc_info->sem_job);
    sem_wait(&module->proc_info->sem_result);

    return NULL;
}


