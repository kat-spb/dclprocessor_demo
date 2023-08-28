#ifndef _PROXY_H_
#define _PROXY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <tcp/client.h>
#include <tcp/server.h>

struct proxy {
    int exit_flag;
    int max_clients;
    struct server *srv;      //only one server in proxy supported
    int clients_cnt;
    struct client **clients; //all clients get all messages from proxy-server requests/replies buffers by command 'status'
    void *internal_data;
};

struct proxy *proxy_create(int max_clients);
void proxy_add_server(struct proxy *px, struct server *srv);
void proxy_add_client(struct proxy *px, struct client *cli);
void proxy_destroy(struct proxy *px);

#ifdef __cplusplus
}
#endif

#endif // _PROXY_H_

