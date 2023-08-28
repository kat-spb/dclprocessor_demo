#ifndef _CLIENT_H_
#define _CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <tcp/connection.h>
#include <tcp/messenger.h>

struct client {
    list_t connection_list;
    int connection_cnt;
    
    int epoll_fd;
    int max_events;
    pthread_t epoll_thread_id;
    int exit_flag;
    sigset_t sigset;
    struct sigaction action;

    struct message *(*read_action_callback)(void *internal_data);

    //struct ring_buffer *requests;
    struct ring_buffer *replies;
    void (*event_callback)(struct connection *conn, struct message *msg);
};

struct client *client_init(); //TODO

struct client *client_create(void (*event_callback)(struct connection *conn, struct message *msg));
void *tcp_client_process(void *arg); //@arg is pointer to 'struct client'

void client_destroy(struct client *cli);

#ifdef __cplusplus
}
#endif

#endif // _CLIENT_H_

