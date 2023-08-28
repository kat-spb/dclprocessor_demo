#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

//network interface and socket
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

//usleep
#include <unistd.h>

#include "misc/list.h"
#include "udpcapture/interface.h"
#include "udpcapture/device.h"
#include "udpcapture/gvcp.h"

struct gvcp_packet *listen_discovery_answer(struct interface *iface,  struct sockaddr *addr) {
     struct sockaddr device_addr;
     socklen_t device_addr_len = sizeof(struct sockaddr);
     fd_set readfd;
     int rc = 0;
     ssize_t bytes = 0;
     struct timeval tv;
     while (1) {
         FD_ZERO(&readfd);
         FD_SET(iface->fd, &readfd);
         memset(&tv, 0, sizeof(struct timeval));
         rc = select(iface->fd + 1, &readfd, NULL, NULL, &tv);
         if (rc < 0) {
             if (errno == EAGAIN || errno == EINTR){
                 continue;
             }
             fprintf(stdout, "[listen_discovery_answer]: fd=%d, select error: %s\n", iface->fd, strerror(errno));
             fflush(stdout);
             close(iface->fd);
             return NULL;
         }
         if (rc == 0){
             break;
         }
         if (rc > 0) {
             if (FD_ISSET(iface->fd, &readfd)) {
                 struct gvcp_packet *packet = (struct gvcp_packet*)malloc(GV_DEVICE_BUFFER_SIZE);
                 bytes = recvfrom(iface->fd, packet, GV_DEVICE_BUFFER_SIZE, 0, &device_addr, &device_addr_len);
                 if (bytes < 0) {
                     if (errno == EAGAIN || errno == EINTR){
                         continue;
                     }
                     fprintf(stdout, "[listen_discovery_answer]: fd=%d, socket read error: %s\n", iface->fd, strerror(errno));
                     fflush(stdout);
                     free(packet);
                     close(iface->fd);
                     return NULL;
                 }
                 if (bytes <= (ssize_t)sizeof(struct gvcp_header)) {
                     fprintf(stdout, "[listen_discovery_answer]: fd=%d, ignore bad packet\n", iface->fd);
                     fflush(stdout);
                     free(packet);
                     continue;
                 }

                 uint32_t ip_local, ip_netmask, ip_remote;
                 ip_local   = ntohl(((struct sockaddr_in*)iface->addr)->sin_addr.s_addr);
                 ip_netmask = ntohl(((struct sockaddr_in*)iface->netmask)->sin_addr.s_addr);
                 ip_remote  = ntohl(((struct sockaddr_in*)&device_addr)->sin_addr.s_addr);
                 fprintf(stdout, "[listen_discovery_answer]: fd = %d, ip_local = 0x%08x, ip_remote = 0x%08x\n", iface->fd, ip_local, ip_remote);
                 fflush(stdout);
                 if (ip_local == ip_remote && (ip_local & ip_netmask) != 0x7f000000) {
                     fprintf(stdout, "[listen_discovery_answer]: fd = %d, ignore local camera on non-loopback interface\n", iface->fd);
                     fflush(stdout);
                     free(packet);
                     continue;
                 }
                 if ((ip_local & ip_netmask) != (ip_remote & ip_netmask)) {
                     fprintf(stdout, "[listen_discovery_answer]: fd = %d, ignore cross-lan camera\n", iface->fd);
                     fflush(stdout);
                     free(packet);
                     continue;
                }
                if (addr) memcpy(addr, &device_addr, sizeof(struct sockaddr));
                return packet;
            }
        }
        else {
            close(iface->fd);
            return NULL;
        }
    }
    close(iface->fd);
    return NULL;
}

void device_print(struct device *dev) {
    fprintf(stdout, "\t======= Device Summary ======\n");
    fprintf(stdout, "\tIP/Port:\t%s:%s\n", dev->ip, dev->port);
    fprintf(stdout, "\tVendor:\t%s\n", dev->vendor);
    fprintf(stdout, "\tModel:\t%s\n", dev->model);
    fprintf(stdout, "\tSerial:\t%s\n", dev->serial);
    fprintf(stdout, "\tUser_id:\t%s\n", dev->user_id);
    fprintf(stdout, "\tMac:\t%s\n", dev->mac);
    fflush(stdout);
}

struct device *device_create(struct sockaddr *iface_addr, struct sockaddr *device_addr) {
    struct device *dev = (struct device *)malloc(sizeof(struct device));
    memset(dev, 0, sizeof(struct device));
    dev->fd = -1;
    if (dev) {
        dev->iface_addr = (struct sockaddr *)malloc(sizeof(struct sockaddr));
        memcpy(dev->iface_addr, iface_addr, sizeof(struct sockaddr));
    }
    int rc = getnameinfo(device_addr, sizeof(struct sockaddr), dev->ip, sizeof(dev->ip), dev->port, sizeof(dev->port), NI_NUMERICHOST | NI_NUMERICSERV);
    if (rc != 0) {
        fprintf(stderr, "can't extract device addr\n");
        fflush(stderr);
        strcpy(dev->ip, "0.0.0.0");
        strcpy(dev->port, "0");
    }
    else {
        fprintf(stdout, "dev->ip = %s dev->port = %s\n", dev->ip, dev->port);
        fflush(stdout);
    }
    return dev;
}

struct device *device_add(struct devices *devs, struct sockaddr *iface_addr, struct sockaddr *device_addr) {
    struct device *dev = device_create(iface_addr, device_addr);
    if (devs) {
        list_add_back(&devs->device_list, &dev->entry);
        devs->device_cnt++;
    }
    return dev;
}

void device_delete(struct devices *devs, struct device *dev){
    if (devs) {
        list_remove_elem(&devs->device_list, &dev->entry);
        devs->device_cnt--;
    }
    close(dev->fd);
    free(dev->iface_addr);
    free(dev);
}

//ATTENTION: memory for devices should be allocated before init
int devices_init(struct devices *devs){
    memset(devs, 0, sizeof(struct devices));
    list_init(&devs->device_list);
    return 0;
}

void devices_destroy(struct devices *devs){
    list_t *item;
    struct device *dev;
    while(!list_is_empty(&devs->device_list)){
        item = list_first_elem(&devs->device_list);
        dev = list_entry(item, struct device, entry);
        device_delete(devs, dev);
    }
}

int devices_discovery(void *internal_data, void (*new_device_callback)(void *internal_data, struct device *dev)) {

    (void)internal_data;
    (void)new_device_callback;

    int cnt = 0;
    struct interfaces *ifaces = prepare_interfaces_list();

    fprintf(stdout, "Detect devices, please, wait 2s...\n");
    fflush(stdout);
    sleep(2); //may be TIMEOUT const???

    struct sockaddr device_addr;

    list_t *elem = list_first_elem(&ifaces->interface_list);
    while(list_is_valid_elem(&ifaces->interface_list, elem)) {
        struct interface *iface = list_entry(elem, struct interface, entry);
        interface_print(iface);
        elem = elem->next;
        struct gvcp_packet *packet = NULL;
        while ((packet = listen_discovery_answer(iface, &device_addr) )!= NULL) {
            struct device *dev = device_create(iface->addr, &device_addr);
            gvcp_discovery_ack(dev->vendor, dev->model, dev->serial, dev->user_id, dev->mac, packet);
            device_print(dev);
            cnt++;
            free(packet);
            free(dev);
        }
    }
    ifaces_destroy(ifaces);
    free(ifaces);
    return cnt;
}

struct devices *prepare_devices_list() {

    struct interfaces *ifaces = prepare_interfaces_list();

    sleep(1); //may be TIMEOUT const???

    struct devices *devs = (struct devices *)malloc(sizeof(struct devices));
    struct sockaddr device_addr;
    devices_init(devs);
    list_t *elem = list_first_elem(&ifaces->interface_list);
    while(list_is_valid_elem(&ifaces->interface_list, elem)) {
        struct interface *iface = list_entry(elem, struct interface, entry);
        interface_print(iface);
        elem = elem->next;
        struct gvcp_packet *packet = NULL;
        while ((packet = listen_discovery_answer(iface, &device_addr) )!= NULL) {
            struct device *dev = device_add(devs, iface->addr, &device_addr);
            gvcp_discovery_ack(dev->vendor, dev->model, dev->serial, dev->user_id, dev->mac, packet);
            device_print(dev);
            free(packet);
        }
    }
    ifaces_destroy(ifaces);
    free(ifaces);
    return devs;
}

//id_string is ip now, we think that they are unique
struct device *device_find(char *id_string) {

    if (!id_string || strcmp("", id_string) == 0) {
        fprintf(stdout, "[device_find]: Device \'%s\' not specified\n", id_string);
        fflush(stdout);
        return NULL;
    }

    struct devices *devs = prepare_devices_list();
    if (devs == NULL) {
        return NULL;
    }

    list_t *elem = list_first_elem(&devs->device_list);
    while (list_is_valid_elem(&devs->device_list, elem)) {
        struct device *dev = list_entry(elem, struct device, entry);
        elem = elem->next;
        if (dev == NULL) {
            devices_destroy(devs);
            free(devs);
            return NULL;
        }
        if (strcmp(dev->ip, id_string) != 0) {
            fprintf(stdout, "Other device \'%s\' found, skip it\n", dev->ip);
            fflush(stdout);
            continue;
        }
        else {
            fprintf(stdout, "Found device for source \'%s\'\n", id_string);
            fflush(stdout);
            return dev;
        }
    }
    fprintf(stdout, "No device with id_string=\'%s\' found\n", id_string);
    fflush(stdout);
    devices_destroy(devs);
    free(devs);
    return NULL;
}


