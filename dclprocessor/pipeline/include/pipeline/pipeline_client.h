#ifndef _PIPELINE_CLIENT_H
#define _PIPELINE_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

struct pipeline_module;
struct pipeline;

//commands part
//@id - ID of module as it set in module_description
//attach/detach to pipeline reinitialized semaphores in process shmem; module process in attach/detach mode SHOULD work with tcp commands as client
int pipeline_module_attach(struct pipeline *pl, struct pipeline_module *module, int id_before, int id_after);
int pipeline_module_detach(struct pipeline *pl, int id);
//process will detached and then killed if it not answer on STATUS request by tcp
int pipeline_module_kill(struct pipeline *pl, int id);
//??? process may be started in attached mode 
int pipeline_module_start(struct pipeline *pl, int id);
//??? process may be stopped in attached mode
int pipeline_module_stop(struct pipeline *pl, int id);

//working in attached mode
typedef void (*pipeline_action_t) (void *internal_data, void *data, int *status);
int pipeline_process(void *internal_data, void (*pipeline_action) (void *internal_data, void *data, int *status));

#ifdef __cplusplus
}
#endif

#endif //_PIPELINE_CLIENT_H
