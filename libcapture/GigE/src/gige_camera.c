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
//usleep
#include <unistd.h>

#include "misc/list.h"

#include "udpcapture/gvcp.h"
#include "udpcapture/gvsp.h"
#include "udpcapture/device.h"
#include "udpcapture/gige_camera.h"

//#include "udpcapture/iframe.h"

struct packet_data {
    int received;
    int64_t time_us;
};

struct frame_data {
    int is_started;
    int is_valid;

    uint64_t frame_id;
    char buffer[MAX_FRAME_SIZE];
    size_t real_size;

    struct gvsp_data_leader leader;

    int has_extended_ids;

    uint32_t n_packets;
    uint32_t error_packets_received;

    uint32_t last_valid_packet;
    struct packet_data *packet_data;

    uint64_t first_packet_time_us;
    uint64_t last_packt_time_us;

    //???
    int n_packets_resent;
    int resend_ratio_reached;
};

void* camera_gvsp_thread(void *arg) {
    int extended_ids = 0;
    uint32_t packet_id = 0;
    uint64_t frame_id = 0;
    uint64_t last_frame_id = 0;
    struct gige_camera *cam = (struct gige_camera *)arg;
    //struct iframe_data *iframe = &cam->iframe;
    int n_received_packets = 0;
    int n_error_packets = 0;
    int n_ignored_packets = 0;
    struct sockaddr device_addr;
    socklen_t device_addr_len = sizeof(struct sockaddr);

    int fd = cam->gvsp_fd;

    //int rc = 0;
    ssize_t bytes = 0;

    struct epoll_event ev;
    int epoll_fd;

    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd < 0){
        fprintf(stderr, "[camera_gvsp_thread]: could not create epoll file descriptor: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

     //add for listen to epoll
    ev.events = EPOLLIN | EPOLLRDHUP;
    ev.data.ptr = &fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) != 0){
        fprintf(stderr, "[camera_gvsp_thread]: could not add file descriptor to epoll set, error: %s\n", strerror(errno));
        return NULL;
    }

    struct frame_data *frame = (struct frame_data*)malloc(sizeof(struct frame_data));
    frame->is_started = 0;

    fprintf(stdout, "[0] Hello from source_data_thread_udp, fd = %d, packet_size = %d\n", fd, cam->packet_size);
    fflush(stdout);
    //cam->packet_size = 4096;

    uint32_t packet_size = cam->packet_size - IP_HEADER_SIZE - UDP_HEADER_SIZE;
    struct gvsp_packet *packet = (struct gvsp_packet*)malloc(packet_size);

    while (cam->state == STREAM_LIVE && cam->gvsp_fd != -1 && cam->gvcp_fd !=-1 ) {
        //fprintf(stdout, "[gvsp_thread]: [1] Hello from source_data_thread_udp\n");
        //fflush(stdout);

        struct epoll_event event;
        int n, timeous_ms = 1000;

        n = epoll_wait(epoll_fd, &event, 1, timeous_ms);
        if (n < 0) {
            if (errno == EAGAIN || errno == EINTR) {
                fprintf(stdout, "[camera_gvsp_thread]: %s on epoll_wait\n", strerror(errno));
                fflush(stdout);
                continue;
            }
            fprintf(stderr, "[camera_gvsp_thread]: could not wait for epoll events, error: %s\n", strerror(errno));
            fflush(stderr);
            close(epoll_fd);
            return NULL;
        }

        if (n == 0) {
            fprintf(stdout, "[camera_gvsp_thread]: no data on inteface\n");
            fflush(stdout);
            continue;
        }

        if (n > 0) {
            bytes = recvfrom(fd, packet, packet_size, 0, &device_addr, &device_addr_len);
            //fprintf(stdout, "[gvsp_thread]: bytes = %ld, packet_size = %d\n", bytes, packet_size);
            //fflush(stdout);
            if (bytes < 0) {
                if (errno == EAGAIN || errno == EINTR) {
                    fprintf(stdout, "[camera_gvsp_thread]: %s on recfrom\n", strerror(errno));
                    fflush(stdout);
                    free(packet);
                    continue;
                }
                fprintf(stdout, "[camera_gvsp_thread]: socket error, %s\n", strerror(errno));
                fflush(stdout);
                free(packet);
                close(fd);
                close(epoll_fd);
                return NULL;
           }
           //work with packet
           n_received_packets++;
           extended_ids = gvsp_packet_has_extended_ids(packet);
           frame_id = gvsp_packet_get_frame_id(packet);
           packet_id = gvsp_packet_get_packet_id(packet);
           //fprintf(stdout, "[gvsp_threadp]: total = %d ", n_received_packets);
           //fprintf(stdout, "packet_id = 0x%hx(%d), frame_id = 0x%lx(%lu)\n", packet_id, packet_id, frame_id, frame_id);
           //fflush(stdout);

           //find frame data
           //now is primitive:  check that frame_id > last_frame_idnew_frame_action(source, &frame_id);
           if (frame_id != last_frame_id) {
               if (frame->is_started) {
                   cam->iframe.frame_id = last_frame_id;
                   if (cam->new_frame_action) cam->new_frame_action(cam->parent, (void*)(&cam->iframe));
               }
               fprintf(stdout, "[camera_gvsp_thread]: new frame %lu found\n", frame_id);
               fflush(stdout);
               (cam->iframe).buffer_size = 0;
               frame->real_size = 0;
               frame->frame_id = frame_id;
               frame->last_valid_packet = -1;
               frame->has_extended_ids = extended_ids;
               frame->is_started = 1;
               frame->n_packets = 0;
               frame->error_packets_received = 0;
               last_frame_id = frame->frame_id;
            }

            if (frame->is_started) {
                enum gvsp_packet_type packet_type = gvsp_packet_get_packet_type(packet);
                if (gvsp_packet_type_is_error(packet_type)) {
                   //TODO:
                    fprintf(stdout, "[gvsp_thread]: error packet recieved\n");
                    fflush(stdout);
                    n_error_packets++;
                    frame->error_packets_received++;
                }
                //TODO: think about necessary of this case
                else if (packet_id < frame->n_packets && frame->packet_data[packet_id].received) {
                    fprintf(stdout, "[gvsp_thread]: very strange -- duplicate packet recieved\n");
                    fflush(stdout);
                }
                else {
                    //fprintf(stdout, "[gvsp_thread]: normal packet recieved\n");
                    //fflush(stdout);
                    enum gvsp_content_type content_type = gvsp_packet_get_content_type(packet);;
                    if (packet_id < frame->n_packets) {
                        frame->packet_data[packet_id].received = 1;
                    }
                    //tracking packets continuious
                    uint32_t i;
                    for (i = frame->last_valid_packet + 1; i < frame->n_packets; i++)
                        if (!frame->packet_data[i].received) break;
                    frame->last_valid_packet = i - 1;

                    switch (content_type) {
                            case GVSP_CONTENT_TYPE_DATA_LEADER: {
                                //process data leader
                                struct gvsp_data_leader *leader = &frame->leader;
                                leader->payload_type = gvsp_packet_get_payload_type(packet);
                                if (leader->payload_type == GVSP_PAYLOAD_TYPE_IMAGE) {
                                    fprintf(stdout, "[gvsp_thread]: image data (timestamp = %lu): width = %u, height = %u, offset_x = %u, offset_y = %u, pixel_format = 0x%x\n",
                                    gvsp_packet_get_timestamp(packet, cam->timestamp_tick_frequency), 
                                    gvsp_packet_get_width(packet),
                                    gvsp_packet_get_height(packet),
                                    gvsp_packet_get_x_offset(packet),
                                    gvsp_packet_get_y_offset(packet),
                                    gvsp_packet_get_pixel_format(packet));
                                fflush(stdout);
                            }
                            else {
                                fprintf(stdout, "[gvsp_thread]: strange payload type\n");
                                fflush(stdout);
                            }
                            break;
                        }
                        case GVSP_CONTENT_TYPE_DATA_BLOCK: {
#if 0
                            size_t block_size;
                            ptrdiff_t block_offset;
                            ptrdiff_t block_end;
                            size_t block_header = sizeof(struct gvsp_packet) + sizeof(struct gvsp_header);
                            block_size = gvsp_packet_get_data_size(packet, packet_size);
                            block_offset = (packet_id - 1) * (packet_size - block_header);
                            block_end = block_size + block_offset;
                            //TODO: block_end vs buffer_size
                            memcpy(frame->buffer + frame->real_size, gvsp_packet_get_data(packet), block_size);
                            frame->real_size += block_size;
#else
                            //process data block -- update this place without copy memory
                            memcpy((cam->iframe).buffer + (cam->iframe).buffer_size, gvsp_packet_get_data(packet), gvsp_packet_get_data_size(packet, packet_size));
                            //memcpy(frame->buffer + frame->real_size, gvsp_packet_get_data(packet), gvsp_packet_get_data_size(packet, packet_size));
                            (cam->iframe).buffer_size += gvsp_packet_get_data_size(packet, packet_size);
                            //frame->real_size += gvsp_packet_get_data_size(packet, packet_size);
#endif
                            //fprintf(stdout, "[gvsp_thread]: frame->real_size = %zd\n", frame->real_size);
                            //fflush(stdout);
                            break;
                        }
                        case GVSP_CONTENT_TYPE_DATA_TRAILER:
#if 0
                            if (frame->error_packets_received > 0) {
                                fprintf(stdout, "[gvsp_thread]: %d error packets recived\n", frame->error_packets_received);
                                fflush(stdout);
                            }
                            else {
                                fprintf(stdout, "[gvsp_thread]: no error packets recieved\n");
                                fflush(stdout);
                            }
#endif
                            fprintf(stdout, "[gvsp_thread]: iframe->real_size = %zd\n", (cam->iframe).buffer_size);
                            //fprintf(stdout, "[gvsp_thread]: frame->real_size = %zd\n", frame->real_size);
                            fflush(stdout);
                            //process data trailer
                            break;
                        default:
                            n_ignored_packets++;
                     }

                    //TODO: check missing packets

                    }
                }
                else {
                    fprintf(stdout, "ignore packet\n");
                    fflush(stdout);
                    n_ignored_packets++;
                }
        }
    }
    cam->last_frame_id = frame_id;
    cam->n_received_packets = n_received_packets;
    cam->n_ignored_packets = n_ignored_packets;
    cam->n_error_packets = n_error_packets;
    free(packet);
    free(frame);
    fprintf(stdout, "[gvsp_thread]: stop\n");
    fflush(stdout);
    cam->state = STREAM_STOP;
    close(epoll_fd);
    return NULL;
}

struct gige_camera_ops *gige_camera_identify_ops(struct device *dev) {
#warning "Hardcode ops by IP instead of camera id (Vendor_Model_Serial)"
    if (strcmp(dev->ip, "127.0.0.1") == 0) {
         return &gige_camera_ops_arvfake;
    }
    if (strcmp(dev->ip, "169.254.2.134") == 0) {
        return  &gige_camera_ops_flir_color;
    }
    if (strcmp(dev->ip, "10.0.1.1") == 0 || strcmp(dev->ip, "10.0.1.2") == 0) {
        return &gige_camera_ops_baumer;
    }
    if (strcmp(dev->ip, "10.0.0.2") == 0 || strcmp(dev->ip, "10.0.1.4") == 0) {
        return &gige_camera_ops_flir_color2;
    }
    if (strcmp(dev->ip, "169.254.0.1") == 0 || strcmp(dev->ip, "10.0.0.3") == 0 ) {
        return  &gige_camera_ops_lucid_polar;
    }
    return &gige_camera_ops_no;
}

//ATTENTION: 'dev' should be not null before init
struct gige_camera *gige_camera_init(struct device *dev, void (*new_frame_action)(void *, void *)) {

    struct gige_camera *cam = (struct gige_camera *)malloc(sizeof(struct gige_camera));

    if (!dev || !cam) {
        fprintf(stderr, "[gige_camera_init]: cam or link to device is NULL\n");
        fflush(stderr);
        return NULL;
    }

    cam->dev = dev;
    cam->ops = gige_camera_identify_ops(cam->dev);
    cam->new_frame_action = new_frame_action;
    cam->gvsp_fd = -1;
    //0xf000 is magic (it should be uint16_t, not 0x0000 and numbers should be increasing)
    cam->last_gvcp_packet_id = 0xf000;

    int fd = -1, rc = 0, opt = 1;

    struct sockaddr_in device_addr;

    //this values are from camera now
    //uint32_t packet_id = 0xf000;
    //uint32_t packet_size = GVSP_PACKET_SIZE_DEFAULT;

    //open command socket (GVCP)
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0) {
        fprintf(stdout, "[gige_camera_init]: gvcp socket error\n");
        fflush(stdout);
        free(cam);
        return NULL;
    }

    //if port is busy we have got a problem with bind
    ((struct sockaddr_in*)dev->iface_addr)->sin_port = htons(0);
    rc = bind(fd, dev->iface_addr, sizeof(struct sockaddr_in));
    if (rc < 0) {
        fprintf(stdout, "[gige_camera_init]: couldn't bind gvcp socket, fd = %d,  %s, %s\n",
            fd, inet_ntoa(((struct sockaddr_in*)dev->iface_addr)->sin_addr), strerror(errno));
        fflush(stdout);
        free(cam);
        return NULL;
    }

    //connect is neccessary for directly write packets to the command socket
    device_addr.sin_family = AF_INET;
    device_addr.sin_addr.s_addr = inet_addr(dev->ip);;
    device_addr.sin_port = htons(atoi(dev->port));
    rc = connect(fd, (struct sockaddr*)&device_addr, sizeof(struct sockaddr_in));
    if (rc) {
        fprintf(stdout, "[gige_camera_init]: couldn't connect to the gvcp socket\n");
        fflush(stdout);
        return NULL;
    }

    cam->gvcp_fd = fd;

    //open data socket (GVSP)
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0) {
        fprintf(stdout, "[gige_camera_init]: gvsp socket error\n");
        fflush(stdout);
        close(cam->gvcp_fd);
        return NULL;
    }

    opt = GV_STREAM_INCOMING_BUFFER_SIZE;
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt));
    rc = bind(fd, dev->iface_addr, sizeof(struct sockaddr_in));
    if (rc < 0) {
        fprintf(stdout, "[gige_camera_init]: bind gvsp socket error, fd = %d, %s, %s\n",
            fd, inet_ntoa(((struct sockaddr_in*)dev->iface_addr)->sin_addr), strerror(errno));
        fflush(stdout);
        close(cam->gvcp_fd);
        close(fd);
        return NULL;
    }

    //gvcp_init returns packet_size from camera
    if ((cam->packet_size = gvcp_init(cam->gvcp_fd, next_packet_id(cam->last_gvcp_packet_id), fd)) > 0) { //think about test camera with acquisition
        cam->gvsp_fd = fd;
        cam->timestamp_tick_frequency = gvcp_get_timestamp_tick_frequency(cam->gvcp_fd, next_packet_id(cam->last_gvcp_packet_id));
#if 0 //wtf, why it's doesn't work when start in this place
        pthread_create(&(cam->data_thread_id), NULL, gvsp_thread, cam);
        fprintf(stdout, "Data thread created on fd = %d\n", cam->gvsp_fd);
        fflush(stdout);
#endif
        return cam;
    }

    return NULL;
}

void gige_camera_destroy(struct gige_camera *cam) {
#if 0
    cam->state = STREAM_STOP;
    sleep(1);
    cam->new_frame_action = NULL;
    pthread_join(cam->data_thread_id, NULL);
#endif
    close(cam->gvsp_fd);
    close(cam->gvcp_fd);
    //if (cam->dev) free(cam->dev);
    //free(camera);  -- this action should be called from place where were malloc()
}

int gige_camera_test(struct gige_camera *cam, int duration) {
    int cnt = 0;
    (cam->ops)->gvcp_start(cam->gvcp_fd, next_packet_id(cam->last_gvcp_packet_id));
    cam->state = STREAM_LIVE;
#if 1
        pthread_create(&(cam->data_thread_id), NULL, camera_gvsp_thread, cam);
        fprintf(stdout, "Data thread created on fd = %d\n", cam->gvsp_fd);
        fflush(stdout);
#endif
    fprintf(stdout, "Start acquisition for %d seconds (fd = %d)\n", duration, cam->gvsp_fd);
    fflush(stdout);
    int always = 0;
    if (duration == 0) always = 1;
    while(always || cnt < duration) {
        (cam->ops)->gvcp_watchdog(cam->gvcp_fd, next_packet_id(cam->last_gvcp_packet_id));
        sleep(1);
        cnt++;
    }
    fprintf(stdout, "Acquisiion for %d seconds finished successfully(fd = %d)\n", duration, cam->gvsp_fd);
    fflush(stdout);

    (cam->ops)->gvcp_stop(cam->gvcp_fd, next_packet_id(cam->last_gvcp_packet_id));
    cam->state = STREAM_STOP;
    close(cam->gvcp_fd);
    cam->gvcp_fd = -1;
    close(cam->gvsp_fd);
    cam->gvsp_fd = -1;
    pthread_join(cam->data_thread_id, NULL);
    return 0;
}


