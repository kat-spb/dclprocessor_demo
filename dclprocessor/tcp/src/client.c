#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <misc/log.h>
#include <misc/list.h>
#include <misc/ring_buffer.h>

#include <tcp/client.h>

void sigusr2_client_handler(int sig){
    (void)sig;
    LOG_FD_INFO("[sigusr2_client_handler]: sigusr2 signal\n");
}

//TODO: now only for old version
struct client *client_init() {
    struct client *cli = (struct client *)malloc(sizeof(struct client));
    memset(cli, 0, sizeof(struct client));
    //cli->state = CLIENT_STOPPED;
    list_init(&cli->connection_list);
    return cli;
}

struct client *client_create(void (*event_callback)(struct connection *conn, struct message *msg)) {
    struct client *cli = (struct client *)malloc(sizeof(struct client));
    memset(cli, 0, sizeof(struct client));
    //cli->state = CLIENT_STOPPED;
    list_init(&cli->connection_list);
    cli->exit_flag = 0;
    cli->max_events = 10;
    cli->event_callback = event_callback;

    sigemptyset(&cli->sigset);
    sigaddset(&cli->sigset, SIGUSR2);
    pthread_sigmask(SIG_BLOCK, &cli->sigset, NULL);
    sigemptyset(&cli->action.sa_mask);
    cli->action.sa_handler = sigusr2_client_handler;
    cli->action.sa_flags = 0;
    sigaction(SIGUSR2, &cli->action, NULL);

    return cli;
}

void client_destroy(struct client *cli) {
    list_t *item;
    struct connection *connection;
    //cli->state = CLIENT_STOPPED;
    while(!list_is_empty(&cli->connection_list)) {
        item = list_first_elem(&cli->connection_list);
        connection = list_entry(item, struct connection, entry);
        connection_delete(&cli->connection_list, &cli->connection_cnt, connection);
    }
    free(cli);
}

