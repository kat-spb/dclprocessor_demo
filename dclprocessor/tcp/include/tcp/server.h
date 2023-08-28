#ifndef _SERVER_H_
#define _SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <tcp/connection.h>
#include <tcp/messenger.h>

struct server {
    list_t connection_list;
    int connection_cnt;

    char port[8];  //listen server port
    int listen_fd; //this server file descriptor
    pthread_t listen_thread_id;

    int epoll_fd;
    int max_events;
    sigset_t sigset;
    struct sigaction action;

    int exit_flag;

    struct ring_buffer *requests;
    void (*execute_callback)(void *internal_data, struct message *msg);
    pthread_t execute_command_thread_id;
    pthread_mutex_t mutex_exec;
    pthread_cond_t cond_exec;
    
    struct ring_buffer *replies;
    void (*send_replies_callback)(void *internal_data, struct connection *conn);
    pthread_t send_replies_thread_id;
    pthread_mutex_t mutex_send;
    pthread_cond_t cond_send;
};

void server_init(struct server **p_srv, char *port);
struct server *server_create(char *port, int max_events, void (*execute_callback)(void *internal_data, struct message *msg), void (*send_replies_callback)(void *internal_data, struct connection *conn));
void server_destroy(struct server *srv);

#ifdef __cplusplus
}
#endif

#endif // _SERVER_H_

