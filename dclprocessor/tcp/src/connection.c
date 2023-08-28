#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <misc/log.h>
#include <tcp/connection.h>

struct connection *connection_add(list_t *connection_list, int *connection_cnt, int fd){
    struct connection *connection = (struct connection *)malloc(sizeof(struct connection));
    if (connection == NULL) return NULL;
    memset(connection, 0, sizeof(struct connection));
    connection->fd = fd;
    if (connection_list) {
        list_add_back(connection_list, &connection->entry);
        (*connection_cnt)++;
    }
    return connection;
}

void connection_delete(list_t *connection_list, int *connection_cnt, struct connection *connection){
    if (connection_list) {
        list_remove_elem(connection_list, &connection->entry);
        (*connection_cnt)--;
    }
    close(connection->fd);
    free(connection);
}

struct connection *connection_get(char *host, char *port, list_t *connection_list){
    list_t *item;
    struct connection *connection;
    item = list_first_elem(connection_list);
    while(list_is_valid_elem(connection_list, item)){
        connection = list_entry(item, struct connection, entry);
        TRACE_N_CONSOLE(INFO, "compare '%s':'%s' vs '%s':'%s'\n", connection->host, connection->service, host, port);
        if (strcmp(connection->service, port) == 0 && strcmp(connection->host, host) == 0) {
            return connection;
        }
        item = item->next;
    }
    return NULL;
}

