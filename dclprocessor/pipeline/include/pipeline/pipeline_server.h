#ifndef _PIPELINE_SERVER_H
#define _PIPELINE_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

//pipeline modules methods
void *initial_thread(void *arg); //for work "in fly"
void *source_thread(void *arg);  //source thread should be first and operation most long by time of all modules
void *process_thread(void *arg);
void *finalization_thread(void *arg);

#ifdef __cplusplus
}
#endif

#endif //_PIPELINE_SERVER_H
