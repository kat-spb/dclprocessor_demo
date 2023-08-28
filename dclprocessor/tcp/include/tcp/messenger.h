#ifndef _MESSENGER_H_
#define _MESSENGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <signal.h>
#include <netdb.h>

#define MAX_MSG_LEN 512

enum MSG_TYPE {
    MSG_UNKNOWN = -1,
    MSG_REQUEST = 0,
    MSG_REPLY = 1
};

enum MSG_CMD {
    MSG_CMD_QUIT,
    MSG_CMD_STATUS,
    MSG_CMD_SET,
    MSG_CMD_GET,
    MSG_CMD_START,
    MSG_CMD_STOP,
    MSG_CMD_INIT,
    MSG_CMD_CONFIG,
    MSG_CMD_CONNECT
};

struct message_actions {
    void (*start_action)(void *internal_data);
    void (*stop_action)(void *internal_data);
    void (*set_action)(void *internal_data, void *values);
    void (*get_action)(void *internal_data, void *values);
    void (*status_action)(void *internal_data, void *values);
};

struct message {
    enum MSG_CMD cmd;
    int cmd_len;
    enum MSG_TYPE type;
    int src_id;
    int dst_id;
    size_t msg_len;
    char msg_string[MAX_MSG_LEN];
    struct message_actions *msg_actions;
};

int msg_get_connect_information(struct message *msg, char **phost, char **pport);
int connection_extract_message(struct connection *conn, struct message *msg);
void *tcp_server_process(void *arg); //@arg is pointer to 'struct server'

#ifdef __cplusplus
}
#endif

#endif// _MESSENGER_H_
