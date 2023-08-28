#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <tcp/server.h>
#include <tcp/client.h>
#include <tcp/proxy.h>

struct proxy *proxy_create(int max_clients) {
    struct proxy *px = (struct proxy *)malloc(sizeof(struct proxy));
    px->max_clients = max_clients;
    px->clients_cnt = 0;
    px->clients = (struct client **)malloc(px->max_clients * sizeof(struct client *) * sizeof(struct client));
    for (int i = 0; i < px->clients_cnt; i++) {
        px->clients[i] = NULL;
    }
    return px;
}

void proxy_add_server(struct proxy *px, struct server *srv) {
    px->srv = srv;
}

void proxy_add_client(struct proxy *px, struct client *cli) {
    px->clients[px->clients_cnt] = cli;
    px->clients_cnt++;
}

void proxy_destroy(struct proxy *px) {
    for (int i = 0; i < px->clients_cnt; i++) {
        struct client *cli = px->clients[i];
        client_destroy(cli);
    }
    free(px->clients);
}

