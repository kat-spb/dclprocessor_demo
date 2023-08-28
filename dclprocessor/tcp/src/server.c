#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <signal.h>

#include <misc/log.h>
#include <misc/list.h>
#include <misc/ring_buffer.h>

#include <tcp/server.h>

//this message will be printed when SIGUSR2 signal on epoll_pwait
void sigusr2_server_handler(int sig){
    (void)sig;
    LOG_FD_INFO("[server, sigusr2_handler]: sigusr2 signal\n");
}

//TODO: now only for old version
void server_init(struct server **p_srv, char *port) {
    if (p_srv == NULL) return;
    struct server *srv = *p_srv;
    if (srv == NULL) {
        srv = (struct server *)malloc(sizeof(struct server));
    }
    
    //TODO: need_to_free
    memset(srv, 0, sizeof(struct server));
    //cli->state = CLIENT_STOPPED;
    list_init(&srv->connection_list);
    strcpy(srv->port, port);
    *p_srv = srv;
}

struct server *server_create(char *port, int max_events, 
                             void (*execute_callback)(void *internal_data, struct message *msg),
                             void (*send_replies_callback)(void *internal_data, struct connection *conn)){
    struct server *srv = (struct server*)malloc(sizeof(struct server));
    memset(srv, 0, sizeof(struct server));
    srv->max_events = max_events;
    srv->exit_flag = 0;
    strcpy(srv->port, port);
    srv->connection_cnt = 0;
    srv->execute_callback = execute_callback;
    srv->send_replies_callback = send_replies_callback;
    list_init(&srv->connection_list);

    sigemptyset(&srv->sigset);
    sigaddset(&srv->sigset, SIGUSR2);
    pthread_sigmask(SIG_BLOCK, &srv->sigset, NULL);
    sigemptyset(&srv->action.sa_mask);
    srv->action.sa_handler = sigusr2_server_handler;
    srv->action.sa_flags = 0;
    sigaction(SIGUSR2, &srv->action, NULL);

    pthread_create(&srv->listen_thread_id, NULL, tcp_server_process, srv);
    return srv;
}

void server_destroy(struct server *srv) {
    list_t *item;
    struct connection *connection;
    //srv->state = SERVER_STOPPED;
    while(!list_is_empty(&srv->connection_list)){
        item = list_first_elem(&srv->connection_list);
        connection = list_entry(item, struct connection, entry);
        connection_delete(&srv->connection_list, &srv->connection_cnt, connection);
    }
    srv->exit_flag = 1;
    //ATTENTION: epoll_wait could be stopped only by signal 
    LOG_FD_INFO("[server_destroy]: sending SIGUSR2 to epoll thread ...\n");

    //TODO: check way when no thread
    pthread_join(srv->listen_thread_id, NULL);

    free(srv);
}

