#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <misc/log.h>
#include <process/process_mngr.h>

#include <pipeline/pipeline.h>
#include <pipeline/pipeline_server.h>

#include <pipeline/pipeline_module.h>

int pipeline_module_init(struct pipeline_module *module, void *shmem, struct ring_buffer *data_ring, int *exit_flag, sem_t *start_control) {
    if (!module) return -1;
    //ATTENTION: Don't set real (module->id) here!
    module->id = -150;
    //module->minfo = NULL;  //Very bad idea -- pipeline_module_init may be after set module->minfo ???
    module->idx = -200;
    module->state = MD_STATE_UNKNOWN;
    module->start_control = start_control;
    module->shmem = shmem;
    sem_init(&(module->new_cmd), 0, 0);
    sem_init(&(module->result), 0, 1);
    module->cmd = PROCESS_CMD_JOB;
    module->last_cmd_result = 0;
    module->data_ring = data_ring;
    module->exit_flag = exit_flag;
    return 0;
}

void pipeline_module_destroy(struct pipeline_module *module) {
    if (!module) return;
    module->id = -150;
    module->idx = -250;
    module->state = MD_STATE_UNKNOWN;
    sem_destroy(&(module->new_cmd));
    sem_destroy(&(module->result));
}

struct pipeline_module *pipeline_module_get(int id, list_t *modules_list){
    list_t *item;
    struct pipeline_module *module;
    item = list_first_elem(modules_list);
    while(list_is_valid_elem(modules_list, item)){
        module = list_entry(item, struct pipeline_module, entry);
        //fprintf(stdout, "[%s]: want = %d vs id = %d [idx = %d]\n", __FUNCTION__, id, module->id, module->idx);
        //fflush(stdout);
        if (module->id == id) {
            fprintf(stdout, "[%s]: found module %d [idx = %d]\n", __FUNCTION__, module->id, module->idx);
            fflush(stdout);
            return module;
        }
        item = item->next;
    }
    return NULL;
}

struct pipeline_module *pipeline_module_add(list_t *modules_list, int *modules_cnt, struct pipeline_module_info *minfo){
    struct pipeline_module *module = (struct pipeline_module *)malloc(sizeof(struct pipeline_module));
    if (module == NULL) return NULL;
    memset(module, 0, sizeof(struct pipeline_module));

    //check module configuration
    //out_pipeline_module_info(minfo);

    module->state = MD_STATE_UNKNOWN;
    module->id = minfo->id;
    module->idx = -1; //thread with module is not started

    module->minfo = minfo; //TODO: think about copy structure
    if (minfo->type == MT_FIRST) {
        if (minfo->mode == MT_MODE_EXTERNAL) {
            module->thread_method = source_thread;
        }
        else {
            module->thread_method = initial_thread;
        }
    }
    else if (minfo->type == MT_LAST) {
        module->thread_method = finalization_thread;
    }
    else if (minfo->type == MT_MIDDLE){ //MT_MIDDLE
        module->thread_method = process_thread;
    }
    else {
        LOG_FD_ERROR("unsupported type %d for module %d, skip it\n", minfo->type, minfo->id);
    }

    //we don't want to fix modules order
    module->next = NULL; //need for update when list will full

    //fprintf(stdout, "[%s]: control output for minfo->id = %d\n", __FUNCTION__, minfo->id);
    //fflush(stdout);
    //pipeline_module_debug_out(module);

    if (modules_list) {
        //TODO: add module in correct place (after/before)
        list_add_back(modules_list, &module->entry);
        (*modules_cnt)++;
    }
    return module;
}

void pipeline_module_delete(list_t *modules_list, int *modules_cnt, struct pipeline_module *module){
    if (modules_list) {
        list_remove_elem(modules_list, &module->entry);
        (*modules_cnt)--;
    }
    if (module->minfo) module->minfo = NULL; //when add set pointer to pdesc->minfos[i]
    free(module);
}

int pipeline_modules_update(list_t *modules_list) {
    int null_found = 0;
    struct pipeline_module *module;
    list_t *item = list_first_elem(modules_list);
    while(list_is_valid_elem(modules_list, item)){
        module = list_entry(item, struct pipeline_module, entry);
        module->next = pipeline_module_get(module->minfo->next, modules_list);
        //TODO: think about usage dfs_check
        if (module->next == NULL) { //only one last module possible
            if (!null_found) {
                null_found = 1;
            }
            else {
                fprintf(stderr, "[%s]: second last (id = %d, next = %d)\n", __FUNCTION__, module->id, module->minfo->next);
                fflush(stderr);
                module->state = MD_STATE_ERROR;
                return -1;
            }
        }
        item = item->next;
    }
    return 0;
}

enum MD_STATE pipeline_module_set_state(enum MD_STATE state, struct pipeline_module *module) {
    enum MD_STATE old_state = module->state;
    if (module->state == MD_STATE_ERROR) return MD_STATE_ERROR;
    module->state = state;
    fprintf(stdout, "[%s, id = %d]: state changed %s(%d) --> %s(%d)\n", __FUNCTION__, module->id, module_state_string(old_state), old_state, module_state_string(state), state);
    fflush(stdout);
    return old_state;
}

void pipeline_modules_set_state(enum MD_STATE state, list_t *modules_list) {
    //TODO: check new state in different cases of current state
    struct pipeline_module *module;
    list_t *item = list_first_elem(modules_list);
    while(list_is_valid_elem(modules_list, item)){
        module = list_entry(item, struct pipeline_module, entry);
        pipeline_module_set_state(state, module);
        item = item->next;
    }
}

void pipeline_module_debug_out(struct pipeline_module *module) {
    fprintf(stdout, "[%s]: module addr = %p\n", __FUNCTION__, module);
    fflush(stdout);
    if (!module) {
        fprintf(stderr, "[%s]: module is NULL\n", __FUNCTION__);
        fflush(stderr);
        return;
    }
    //TODO: print human-readable values idx and status as function for enum
    fprintf(stdout, "\t{{id=%d}, {idx=%d},\n"
                    "\t{state=%s(%d)},\n"
                    "\t{thread_id=%lx}, {thread_method=%p}, \n",
                    module->id, module->idx, 
                    module_state_string(module->state), module->state,
                    module->thread_id, module->thread_method);
    if (module->exit_flag == NULL) {
        fprintf(stdout, "\t{exit_flag=NULL},\n");
    }
    else {
        fprintf(stdout, "\t{exit_flag=%p}, {*exit_flag=%d},\n",
                    module->exit_flag, *(module->exit_flag));
    }
    if (module->next == NULL) {
        fprintf(stdout, "\t{next=NULL}\n");
    }
    else {
        fprintf(stdout, "\t{next=%p: {next_id=%d}, {next_idx=%d}}\n",
                    module->next, module->next->id, module->next->idx);
    }
    fflush(stdout);
}

void pipeline_modules_list_debug_out(list_t *modules_list) {
    struct pipeline_module *module;
    list_t *item = list_first_elem(modules_list);
    while(list_is_valid_elem(modules_list, item)) {
        module = list_entry(item, struct pipeline_module, entry);
        pipeline_module_debug_out(module);
        item = item->next;
    }
}

