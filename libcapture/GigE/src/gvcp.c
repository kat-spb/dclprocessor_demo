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
#include <sys/epoll.h>

#include <netdb.h>
#include <arpa/inet.h>


//open/write
#include <sys/stat.h>
#include <fcntl.h>

//usleep
#include <unistd.h>

#include "udpcapture/gvcp.h"

#define DEFAULT_PACKET_SIZE 8000 //4096

int get_net_parameters_from_socket(int fd, uint32_t *ip_value, uint32_t *port_value) {
    int rc = 0;
    //actual for data_fd, set ip and port value as register
    char ip[16]; //my ip
    unsigned int port; //my port
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    //get source IP and port from socket -- for set ip:port to camera registers
    rc = getsockname(fd, (struct sockaddr *)&addr, &addr_len); 
    if (rc < 0) {
        fprintf(stdout, "[get_net_parameters_from_socket]: error with getsockname\n");
        fflush(stdout);
        return -1;
    }
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    port = ntohs(addr.sin_port);
    fprintf(stdout, "[get_net_parameters_from_socket]: local stream socket %d address is %s:%u\n", fd, ip, port);
    fflush(stdout);

    *ip_value = ntohl(addr.sin_addr.s_addr);
    *port_value = port;

    return 0;
}

void print_packet_full(struct gvcp_packet *packet) {
    struct gvcp_header *header = (struct gvcp_header*)packet; 
    char *bytes = (char*)packet;
    uint32_t i, packet_size = sizeof(struct gvcp_packet) + ntohs(header->size);
    fprintf(stdout, "packet: ");
    fflush(stdout);
    for (i = 0; i < packet_size; i++) {
        fprintf(stdout, "%02hhx ", bytes[i]);
        fflush(stdout);
    }
    fprintf(stdout, "\n");
    fflush(stdout);
}

void print_packet_header(struct gvcp_packet *packet) {
    char *bytes = (char*)packet;
    uint32_t i, packet_size = sizeof(struct gvcp_packet);
    fprintf(stdout, "header: ");
    fflush(stdout);
    for (i = 0; i < packet_size; i++) {
        fprintf(stdout, "%02hhx ", bytes[i]);
        fflush(stdout);
    }
    fprintf(stdout, "\n");
    fflush(stdout);
}

void print_packet_data(struct gvcp_packet *packet) {
    struct gvcp_header *header = (struct gvcp_header*)packet; 
    char *bytes = (char*)packet->data;
    uint32_t i, packet_size = ntohs(header->size);
    fprintf(stdout, "data: ");
    fflush(stdout);
    for (i = 0; i < packet_size; i++) {
        fprintf(stdout, "%02hhx ", bytes[i]);
        fflush(stdout);
    }
    fprintf(stdout, "\n");
    fflush(stdout);
}

struct gvcp_packet *gvcp_discovery_packet(size_t *packet_size) {
    *packet_size = sizeof(struct gvcp_header);
    struct gvcp_packet *packet = (struct gvcp_packet *)malloc(*packet_size); 
    //magic from aravis (see wireshark): protocol GVCP(GigE Vision Communication Protocol) on port 3956
    packet->header.packet_type = GVCP_PACKET_TYPE_CMD;
    packet->header.packet_flags = GVCP_CMD_PACKET_FLAGS_ACK_REQUIRED;
    packet->header.command = htons(GVCP_COMMAND_DISCOVERY_CMD);
    packet->header.size = htons(0x0000);
    packet->header.id = htons(0xffff);
    return packet;
}

int gvcp_discovery_ack(char *vendor, char *model, char *serial, char *user_id, char *mac, struct gvcp_packet *packet) {
    if (!packet) return -1;
    struct gvcp_header *header = &packet->header;
    char *data = packet->data;
    if ((ntohs(header->command) == GVCP_COMMAND_DISCOVERY_ACK) && (ntohs(header->id) == 0xffff)){
        if (vendor) strcpy(vendor, data + GVCP_MANUFACTURER_NAME_OFFSET);
        if (model) strcpy(model, data + GVCP_MODEL_NAME_OFFSET);
        if (serial) strcpy(serial, data + GVCP_SERIAL_NUMBER_OFFSET);
        if (user_id) strcpy(user_id, data + GVCP_USER_DEFINED_NAME_OFFSET);
        if (mac) sprintf(mac, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", 
                data[GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 2],
                data[GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 3],
                data[GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 4],
                data[GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 5],
                data[GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 6],
                data[GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 7]);
    }
    return 0;
}

struct gvcp_packet *gvcp_read_register_packet(uint32_t reg_addr, uint32_t packet_id, uint32_t *packet_size) {
    *packet_size = sizeof(struct gvcp_header) + sizeof(uint32_t);
    struct gvcp_packet *packet = (struct gvcp_packet *)malloc(*packet_size); 
    //protocol GVCP(GigE Vision Communication Protocol) on port 3956
    packet->header.packet_type = GVCP_PACKET_TYPE_CMD;
    packet->header.packet_flags = GVCP_CMD_PACKET_FLAGS_ACK_REQUIRED;
    packet->header.command = htons(GVCP_COMMAND_READ_REG_CMD);
    packet->header.size = htons(sizeof(uint32_t));
    packet->header.id = htons(packet_id);

    uint32_t n_addr = htonl(reg_addr);
    memcpy(&packet->data, &n_addr, sizeof(uint32_t));

    fprintf(stdout, "[readreg]: command = 0x%04x reg_addr = 0x%04x\n", ntohs(packet->header.command), reg_addr);
    fflush(stdout);
    return packet;
}

struct gvcp_packet *gvcp_write_register_packet(uint32_t reg_addr, uint32_t reg_value, uint32_t packet_id, uint32_t *packet_size) {
    *packet_size = sizeof(struct gvcp_header) + 2 * sizeof(uint32_t);
    struct gvcp_packet *packet = (struct gvcp_packet *)malloc(*packet_size); 
    //protocol GVCP(GigE Vision Communication Protocol) on port 3956
    packet->header.packet_type = GVCP_PACKET_TYPE_CMD;
    packet->header.packet_flags = GVCP_CMD_PACKET_FLAGS_ACK_REQUIRED;
    packet->header.command = htons(GVCP_COMMAND_WRITE_REG_CMD);
    packet->header.size = htons(2 * sizeof(uint32_t));
    packet->header.id = htons(packet_id);

    uint32_t n_addr = htonl(reg_addr);
    memcpy(&packet->data[0], &n_addr, sizeof(uint32_t));
    uint32_t n_value = htonl(reg_value);
    memcpy(&packet->data[sizeof(uint32_t)], &n_value, sizeof(uint32_t));

    fprintf(stdout, "[writereg]: command = 0x%04x reg_addr = 0x%04x value = 0x%04x\n", ntohs(packet->header.command), reg_addr, reg_value);
    fflush(stdout);
    return packet;
}

int gvcp_register_ack(uint32_t *value, struct gvcp_packet *packet) {
    if (!value || !packet) return -1;
    struct gvcp_header *header = &packet->header;
    char *data = (char *)packet->data;
    //print_packet_data(packet);
    if (ntohs(header->packet_type) == GVCP_PACKET_TYPE_ACK){
        *value = ntohl(*(uint32_t*)data);
    }
    fprintf(stdout, "[ack]: command = 0x%04x value = 0x%04x\n", ntohs(header->command), *value);
    fflush(stdout);
    return 0;
}

struct gvcp_packet *listen_packet_ack(int fd) {
    struct sockaddr device_addr;
    socklen_t device_addr_len = sizeof(struct sockaddr);
    fd_set readfd;
    int rc = 0;
    ssize_t bytes = 0;
    struct timeval tv;
    while (fd != -1) {
        FD_ZERO(&readfd);
        FD_SET(fd, &readfd);
        tv.tv_sec = 0;
        tv.tv_usec = 500000;
        rc = select(fd + 1, &readfd, NULL, NULL, &tv);
        if (rc > 0) {
            if (FD_ISSET(fd, &readfd)) {
                uint32_t packet_size = sizeof(struct gvcp_header);
                //for read and write register
                packet_size += sizeof(uint32_t);
                struct gvcp_packet *packet = (struct gvcp_packet*)malloc(packet_size);
                bytes = recvfrom(fd, packet, packet_size, 0, &device_addr, &device_addr_len);
                if (bytes < 0) {
                    fprintf(stdout, "[listen_packet_ack]: socket error\n");
                    fflush(stdout);
                    free(packet);
                    return NULL;
                }
                if (bytes < packet_size) {
                    fprintf(stdout, "[listen_packet_ack]: wrong on inteface\n");
                    fflush(stdout);
                    continue;
                }
                return packet;
            }
        }
        else {
            //usleep(100);
            //continue;
            return NULL;
        }
    }
    return NULL;
}

struct gvcp_packet *listen_packet_ack_new(int listen_fd) {
    struct sockaddr device_addr;
    socklen_t device_addr_len = sizeof(struct sockaddr);
    struct epoll_event ev;
    int epoll_fd;
    ssize_t bytes = 0;

    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd < 0){
        fprintf(stderr, "[listen_packet_ack_new]: could not create epoll file descriptor: %s\n", strerror(errno));
        return NULL;
    }

    //add for listen to epoll
    ev.events = EPOLLIN | EPOLLRDHUP;
    ev.data.ptr = &listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) != 0){
        fprintf(stderr, "Could not add file descriptor to epoll set, error: %s\n", strerror(errno));
        return NULL;
    }

    while (listen_fd != -1) {
        struct epoll_event event;
        int n, timeous_ms = 500;

        n = epoll_wait(epoll_fd, &event, 1, timeous_ms);
        if (n < 0) {
            fprintf(stderr, "[listen_packet_ack_new]: could not wait for epoll events, error: %s\n", strerror(errno));
            fflush(stderr);
            close(epoll_fd);
            return NULL;
        }

        if (n == 0) {
            fprintf(stdout, "[listen_packet_ack_new]: no data\n");
            fflush(stdout);
            close(epoll_fd);
            return NULL;
            //continue;
        }

        if (n > 0) {
            uint32_t packet_size = sizeof(struct gvcp_header);
            //for read and write register
            packet_size += sizeof(uint32_t);
            struct gvcp_packet *packet = (struct gvcp_packet*)malloc(packet_size);
            bytes = recvfrom(listen_fd, packet, packet_size, 0, &device_addr, &device_addr_len);
            if (bytes < 0) {
                fprintf(stdout, "[listen_packet_ack_new]: socket error\n");
                fflush(stdout);
                free(packet);
                close(epoll_fd);
                return NULL;
            }
            if (bytes < packet_size) {
                fprintf(stdout, "[listen_packet_ack]: wrong on inteface\n");
                fflush(stdout);
                continue;
            }
            close(epoll_fd);
            return packet;
        }
    }
    close(epoll_fd);
    return NULL;
}

int listen_all_packets_ack_new(int listen_fd) {
    struct sockaddr device_addr;
    socklen_t device_addr_len = sizeof(struct sockaddr);
    struct epoll_event ev;
    int epoll_fd;
    int cnt = 0;
    ssize_t bytes = 0;

    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd < 0){
        fprintf(stderr, "[listen_packet_ack_new]: could not create epoll file descriptor: %s\n", strerror(errno));
        return 0;
    }

    //add for listen to epoll
    ev.events = EPOLLIN | EPOLLRDHUP;
    ev.data.ptr = &listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) != 0){
        fprintf(stderr, "Could not add file descriptor to epoll set, error: %s\n", strerror(errno));
        return 0;
    }

    while (listen_fd != -1) {
        struct epoll_event event;
        int n, timeous_ms = 200;

        n = epoll_wait(epoll_fd, &event, 1, timeous_ms);
        if (n < 0) {
            fprintf(stderr, "[listen_packet_ack_new]: could not wait for epoll events, error: %s\n", strerror(errno));
            fflush(stderr);
            close(epoll_fd);
            return 0;
        }

        if (n == 0) {
            fprintf(stdout, "[listen_packet_ack_new]: no data\n");
            fflush(stdout);
            close(epoll_fd);
            return 0;
        }

        if (n > 0) {
            uint32_t packet_size = sizeof(struct gvcp_header);
            //for read and write register
            packet_size += sizeof(uint32_t);
            struct gvcp_packet *packet = (struct gvcp_packet*)malloc(packet_size);
            bytes = recvfrom(listen_fd, packet, packet_size, 0, &device_addr, &device_addr_len);
            if (bytes < 0) {
                fprintf(stdout, "[listen_packet_ack_new]: socket error\n");
                fflush(stdout);
                free(packet);
                close(epoll_fd);
                return 0;
            }
            if (bytes < packet_size) {
                fprintf(stdout, "[listen_packet_ack]: wrong on inteface\n");
                fflush(stdout);
                continue;
            }
            free(packet);
            cnt++;
        }
    }
    close(epoll_fd);
    return cnt;
}

//important: read/write commands are possible only after connect to command socket
int read_register(int fd, uint32_t reg_addr, uint32_t *value, uint32_t packet_id) {
    uint32_t packet_size = 0;
    struct gvcp_packet *packet = gvcp_read_register_packet(reg_addr, packet_id, &packet_size);
    //print_packet_full(packet);
    int rc = write(fd, packet, packet_size);
    free(packet);
    //usleep(500);
    //packet = listen_packet_ack(fd);
    packet = listen_packet_ack_new(fd);
    gvcp_register_ack(value, packet);
    //print_packet_full(packet);
    free(packet);
    return rc;
}

//important: read/write commands are possible only after connect to command socket
int write_register(int fd, uint32_t reg_addr, uint32_t value, uint32_t packet_id) {
    uint32_t packet_size = 0;
    struct gvcp_packet *packet = gvcp_write_register_packet(reg_addr, value, packet_id, &packet_size);
    //print_packet_full(packet);
    int rc = write(fd, packet, packet_size);
    free(packet);
    //usleep(500);
    //packet = listen_packet_ack(fd);
    packet = listen_packet_ack_new(fd);
    gvcp_register_ack(&value, packet);
    //print_packet_full(packet);
    free(packet);
    return rc;
}

int write_watchdog_register(int fd, uint32_t reg_addr, uint32_t value, uint32_t packet_id) {
    uint32_t packet_size = 0;
    struct gvcp_packet *packet = gvcp_write_register_packet(reg_addr, value, packet_id, &packet_size);
    //print_packet_full(packet);
    int rc = write(fd, packet, packet_size);
    free(packet);
    rc = listen_all_packets_ack_new(fd);
    return rc;
}


uint64_t gvcp_get_timestamp_tick_frequency(int fd, uint32_t packet_id) {
    //Read camera parameters
    uint32_t timestamp_tick_frequency_high;
    uint32_t timestamp_tick_frequency_low;

    read_register(fd, GV_TIMESTAMP_TICK_FREQUENCY_HIGH_OFFSET, &timestamp_tick_frequency_high, next_packet_id(packet_id));
    read_register(fd, GV_TIMESTAMP_TICK_FREQUENCY_LOW_OFFSET, &timestamp_tick_frequency_low, next_packet_id(packet_id));
    return ((uint64_t) timestamp_tick_frequency_high << 32) | timestamp_tick_frequency_low;
}

uint32_t gvcp_get_packet_size(int fd, uint32_t packet_id) {
    //Read camera parameters
    uint32_t packet_size;
    read_register(fd, GV_STREAM_CHANNEL_0_PACKET_SIZE_OFFSET, &packet_size, next_packet_id(packet_id));
    return packet_size;
}

uint32_t gvcp_init(int fd, uint32_t packet_id, int data_fd) {
    uint32_t value = -1;
    uint32_t packet_size = 0;

    uint32_t ip, port;
    get_net_parameters_from_socket(data_fd, &ip, &port);

    //Read a number of stream channels register from the camera socket
    read_register(fd, GV_REGISTER_N_STREAM_CHANNELS_OFFSET, &value, packet_id);
    read_register(fd, GV_GVCP_CAPABILITY_OFFSET, &value, next_packet_id(packet_id));

    //Control Channel Privelege on
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
    //Write Stream Channel 0 Destination address, port and packet_size
    write_register(fd, GV_STREAM_CHANNEL_0_IP_ADDRESS_OFFSET, ip, next_packet_id(packet_id));
    write_register(fd, GV_STREAM_CHANNEL_0_PORT_OFFSET, port, next_packet_id(packet_id));
    read_register(fd, GV_STREAM_CHANNEL_0_PACKET_SIZE_OFFSET, &packet_size, next_packet_id(packet_id));

    //HACK:
    if (packet_size > DEFAULT_PACKET_SIZE) {
        packet_size = DEFAULT_PACKET_SIZE;
        write_register(fd, GV_STREAM_CHANNEL_0_PACKET_SIZE_OFFSET, packet_size, next_packet_id(packet_id));
    }

    //return packet_size from channel
    return packet_size;
}

//TODO: gvcp_genicam_xml()

//TODO: gvcp_apply_command_sequence()




