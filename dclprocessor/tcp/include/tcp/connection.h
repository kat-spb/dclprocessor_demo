#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <misc/list.h>
#include <netdb.h>

#define MAX_MSG_LEN 512

struct connection {
    list_t entry;
    int broken;
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
    int fd; //this connection file descriptor
    int epoll_fd; //-1 if not in epoll set
// for new code
    size_t buffer_size;
    size_t offset;
    char buffer[MAX_MSG_LEN + 1];
// for old code
    char request[MAX_MSG_LEN + 1];
    size_t request_used;
    char reply[MAX_MSG_LEN + 1];
    size_t reply_used;
};

struct connection *connection_add(list_t *connection_list, int *connection_cnt, int fd);
void connection_delete(list_t *connection_list, int *connection_cnt, struct connection *connection);
struct connection *connection_get(char *host, char *port, list_t *connection_list);

#ifdef __cplusplus
}
#endif

#endif // _CONNECTION_H_

