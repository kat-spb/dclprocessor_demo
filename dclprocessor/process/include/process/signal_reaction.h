#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

void signal_thread_init(int *exit_flag);
void signal_thread_destroy(void);
void set_signal_reactions(void);

#ifdef __cplusplus
}
#endif

#endif //_SIGNAL_H_
