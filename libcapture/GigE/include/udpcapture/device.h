#ifndef _DEVICE_H_
#define _DEVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

//this about what is necessary
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "misc/list.h"
#include "udpcapture/gvcp.h"

//think about without sockaddr in structure
struct device {
    list_t entry;

    int fd; //?interface

    char ip[20];
    char port[8];

    struct sockaddr host_addr;
    struct sockaddr_in device_addr;

    char vendor[GVCP_MANUFACTURER_NAME_SIZE + 1];
    char model[GVCP_MODEL_NAME_SIZE + 1];
    char serial[GVCP_SERIAL_NUMBER_SIZE + 1];
    char user_id[GVCP_USER_DEFINED_NAME_SIZE + 1];
    char mac[18];

    char iface_name[80];
    struct sockaddr *iface_addr;
};

//struct device *device_add(struct devices *devs, struct sockaddr *iface_addr, struct sockaddr *device_addr);
//void device_delete(struct devices *devs, struct device *dev);
void device_print(struct device *dev);

struct devices {
    int device_cnt;
    list_t device_list;
};

//ATTENTION: memory for devices should be allocated before init
int devices_init(struct devices *devs);
void devices_destroy(struct devices *devs);

struct devices *prepare_devices_list();

int devices_discovery(void *internal_data, void (*new_device_callback)(void *internal_data, struct device *dev)); 

//full cycle of find on all intefraces (= always actual list of device)
struct device *device_find(char *id_string);

int devices_all();
 
#ifdef __cplusplus
}
#endif

#endif //_DEVICE_H_
