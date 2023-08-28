#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>

#include <sys/uio.h>

#include <semaphore.h>
#include <pthread.h>

#include <collector/source.h>

//TODO: think about interface to camera file as 'gigecam.h'
#include <udpcapture/iframe.h>
#include <udpcapture/gvsp.h>
#include <udpcapture/gvcp.h>
#include <udpcapture/device.h>
#include <udpcapture/gige_camera.h>

void new_frame_action(void *parent_data, void *iframe_data) {
    if (!parent_data) {
        fprintf(stdout, "[new_frame_action]: no parent, WTF?\n");
        fflush(stdout);
        return;
    }
    struct data_source *source = (struct data_source*)parent_data;
    struct data_packet *packet = source->packet;
    int frame_id = -1;
    if (source->camera == NULL) { //tcp fake source
        frame_id = *((int *)iframe_data);
    }
    else { //GigE udp real camera or aravis fake camera
        struct iframe_data *iframe = (struct iframe_data *)iframe_data;
        packet->data = iframe->buffer;
        frame_id = (int)(iframe->frame_id);
    }
    //fprintf(stdout, "[new_frame_action, %d]: I'm  in new_fram_action callback of source\n", source->sid);
    //fflush(stdout);
    //---------------------------------------------------------------------------------------
    // Detect new object
    //----------------------------------------------------------------------------------------
    if ((source->object_detection_callback != NULL && source->object_detection_callback(source->internal_data, source->sid, frame_id, (void*)packet->data)) || (source->frames_in_object % source->object_detection_frame_limit == 0)) {
        source->frames_in_object = 0;
        //fprintf(stdout, "[source_data_thread, %d]: I'm  in source thread before wait(source:data)\n", source->sid);
        //fflush(stdout);
        sem_post(&source->data_ready);
        //fprintf(stdout, "[source_data_thread, %d]: I'm  in source thread after wait(source:data)\n", source->sid);
        //fflush(stdout);
        if (!source->exit_flag) {
            //fprintf(stdout, "[source_data_thread, %d]: I'm  in source thread before wait(source:empty)\n", source->sid);
            //fflush(stdout);
            sem_wait(&source->empty_ready);
            //fprintf(stdout, "[source_data_thread, %d]: I'm  in source thread after wait(source:empty)\n", source->sid);
            //fflush(stdout);
        }
    }
    //fprintf(stdout, "[source_data_thread, %d]: I'm  after object_detection_callback\n", source->sid);
    //fflush(stdout);
    //---------------------------------------------------------------------------------------
    // Add frame to the object memory
    //----------------------------------------------------------------------------------------
    //fprintf(stdout, "[source_data_thread, %d]: I'm  before add_frame_callback\n", source->sid);
    //fflush(stdout);
    if (!source->exit_flag && source->add_frame_callback) source->add_frame_callback(source->internal_data, source->sid, frame_id, (void*)packet->data);
    //fprintf(stdout, "[source_data_thread, %d]: I'm  after add_frame_callback\n", source->sid);
    //fflush(stdout);
    //---------------------------------------------------------------------------------------
    // Save to drive (debug)
    //----------------------------------------------------------------------------------------
    //fprintf(stdout, "[source_data_thread, %d]: I'm  before save_frame_callback\n", source->sid);
    //fflush(stdout);
    if (source->save_frame_callback) source->save_frame_callback(source->internal_data, source->sid, frame_id, (void*)packet->data);
    //fprintf(stdout, "[source_data_thread, %d]: I'm  after save_frame_callback\n", source->sid);
    //fflush(stdout);
     source->frames_in_object++;
}

void new_device_callback(void *internal_data, struct device *dev) {
    struct gige_camera *cam = gige_camera_init(dev, new_frame_action);
    (void)internal_data;
    fprintf(stdout, "I'm in the new device callback\n");
    fflush(stdout);
    device_print(dev);
    free(cam);
}

void *source_data_thread_udp(void *arg){
    struct data_source *source = (struct data_source*)arg;
    //struct source_description *sdesc = (struct source_description *)(source->sdesc);;
    struct gige_camera *cam = source->camera;

    //TODO: strange place for this action
    cam->parent = (void*)source;

    sem_wait(&source->empty_ready);
    (cam->ops)->gvcp_start(cam->gvcp_fd, next_packet_id(cam->last_gvcp_packet_id));
    cam->state = STREAM_LIVE;
#if 1
    pthread_create(&(cam->data_thread_id), NULL, camera_gvsp_thread, cam);
    fprintf(stdout, "[source_data_thread_udp, %d]: Data thread created on fd = %d\n", source->sid, cam->gvsp_fd);
    fflush(stdout);
#endif
    while(!source->exit_flag && cam->state == STREAM_LIVE) {
        //fprintf(stdout, "[source_data_thread_udp, %d]: I'm in watchdog cycle on fd = %d\n", source->sid, cam->gvcp_fd);
        //fflush(stdout);
        (cam->ops)->gvcp_watchdog(cam->gvcp_fd, next_packet_id(cam->last_gvcp_packet_id));
        usleep(250000);
        //fprintf(stdout, "[source_data_thread_udp, %d]: source->exit_flag = %d\n", source->sid, source->exit_flag);
        //fflush(stdout);
    }
    //fprintf(stdout, "[source_data_thread_udp, %d]: I'm after watchdog cycle on fd = %d, stopping source...\n", source->sid, cam->gvcp_fd);
    //fflush(stdout);
    cam->state = STREAM_STOP;
    (cam->ops)->gvcp_stop(cam->gvcp_fd, next_packet_id(cam->last_gvcp_packet_id));
    source->exit_flag = 1;
    sem_post(&source->data_ready);
    close(cam->gvcp_fd);
    cam->gvcp_fd = -1;
#if 1
    sleep(1);
    pthread_join(cam->data_thread_id, NULL);
#endif
    fprintf(stdout, "[source_data_thread_udp, %d]: stop\n", source->sid);
    fflush(stdout);
    return NULL;
}

void *source_data_thread_tcp(void *arg){

    struct data_source *source = (struct data_source*)arg;
    struct source_description *sdesc = (struct source_description *)(source->sdesc);
    struct data_packet *packet = source->packet;

    int fd = source->fd;

    size_t  data_size = 0;
    int bytes = 0;
    size_t recieved_len = 0, remaining_len = packet->header_len;
    int frame_id = -1;

    int header_ok = 0;

    //fprintf(stdout, "[source_data_thread, %d]: I'm  in source thread before wait(source:empty)\n", source->sid);
    //fflush(stdout);
    sem_wait(&source->empty_ready);
    while (!(source->exit_flag)) {
        if (remaining_len == 0) {
            //fprintf(stdout, "wtf???\n");
            //fflush(stdout);
            remaining_len = packet->header_len;
            continue;
        }
        if (!header_ok) {
            //fprintf(stdout, "2\n");
            //fflush(stdout);
            bytes = read(fd, (char*)packet->header + recieved_len, remaining_len);
        }
        else {
            //fprintf(stdout, "3: recv = %zd, rem = %zd\n", recieved_len, remaining_len);
            //fflush(stdout);
            bytes = read(fd, (char*)packet->data + recieved_len, remaining_len);
        }
        if (bytes < 0) {
            //fprintf(stdout, "[tcp_capture_data_process]: error while frame_size read\n");
            //fflush(stdout);
            source->exit_flag = 1;
            break;
        }
        else if (bytes == 0) {
            usleep(1000);
            continue;
        }
        else {
            //fprintf(stdout, "header=%d bytes=%zd recieved=%zd remining=%zd\n", header_ok, bytes, recieved_len, remaining_len);
            //fflush(stdout);
            recieved_len += bytes;
            if (!header_ok && recieved_len < packet->header_len) {
                remaining_len -= bytes;
            }
            if (!header_ok && recieved_len == packet->header_len) {
                data_size = *((size_t*)packet->header);
                frame_id = *((int*)((char*)packet->header + sizeof(size_t)));
                //fprintf(stdout, "[source_data_thread]: found_image_size = %zd found_frame_id = %d\n", data_size, frame_id);
                //fflush(stdout);
                if (data_size != sdesc->data_len + sdesc->header_len) {
                    fprintf(stderr, "[source_data_thread]: strange data size = %zd instead of %zd\n", data_size, (size_t)(sdesc->data_len));
                    fflush(stderr);
                }
                recieved_len = 0;
                remaining_len = data_size;
                header_ok = 1;
            }
            if (header_ok && recieved_len < packet->data_len) {
                remaining_len -= bytes;
            }
            if (header_ok && recieved_len == packet->data_len) {
                remaining_len = packet->header_len;
                recieved_len = 0;
                header_ok = 0;
                new_frame_action(source, &frame_id);
            }
        }
    }
    sem_post(&source->data_ready);
    fprintf(stdout, "[source_data_thread, %d]: stop\n", source->sid);
    fflush(stdout);
    return NULL;
}

int source_init(struct data_source **psource, struct source_description *sdesc) {

    if (!psource) {
        fprintf(stderr, "[source_init]: null pointer\n");
        fflush(stderr);
        return -1;
    }

    struct data_source *source = *psource;

    if (source == NULL) {
        source = (struct data_source *)malloc(sizeof(struct data_source));
        source->need_to_free = 1;
    }
    else {
        source->need_to_free = 0;
    }

    if (!source) {
        fprintf(stderr, "[source_init]: can't allocte source\n");
        fflush(stderr);
        return -1;
    }

    source->sdesc = sdesc;
    fprintf(stdout, "[source_init, %s]: check source configuration\n", sdesc->port);
    fflush(stdout);
    out_source_description(source->sdesc);

    source->object_detection_frame_limit = OBJECT_DETECTION_FRAME_LIMIT;
    source->frames_in_object = 0;
    source->exit_flag = 0;
    source->fd = -1;
    source->camera = NULL;

    //TODO: use only this structure or only structure iframe
    struct data_packet *packet = (struct data_packet *)malloc(sizeof(struct data_packet));
    source->packet = packet;
    packet->header = (char *)malloc(sdesc->header_len * sizeof(char));
    packet->header_len = sdesc->header_len;
    packet->data_len = sdesc->data_len; //sdesc->data_len;
    packet->data = (char*)malloc(sdesc->data_len);

    if (sdesc->type == STYPE_TCP) {
        struct addrinfo hints;
        struct addrinfo *result, *rp;
        int rc, fd;

        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;     //IPv4 && IPv6
        hints.ai_socktype = SOCK_STREAM; //TCP socket
        hints.ai_flags = 0;
        hints.ai_protocol = 0;           //Any protocol

        rc = getaddrinfo(sdesc->addr, sdesc->port, &hints, &result);
        if (rc != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
            return -1;
        }

        for (rp = result; rp != NULL; rp = rp->ai_next) {
            fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (fd == -1) continue;
            if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1) break;
            close(fd);
        }

        if (rp == NULL) {
            fprintf(stderr, "Could not connect\n");
            return -1;
        }

        freeaddrinfo(result);
        source->fd = fd;
    }
    else {
        struct device *dev = device_find(sdesc->sid_string);
        if (!dev) {
            return -1;
        }
        fprintf(stdout, "[source_init, %d]: sid_string = \'%s\'; ip = \'%s\'\n", source->sid, sdesc->sid_string, dev->ip);
        fflush(stdout);
        source->camera = gige_camera_init(dev, new_frame_action);
    }

    source->exit_flag = 0;
    source->object_detection_callback = NULL;
    source->add_frame_callback = NULL;
    source->save_frame_callback = NULL;

    if (sem_init(&(source->data_ready), 1, 0) == -1) {
        fprintf(stderr, "[source %d]: can't init source semaphore (ready)\n", source->sid);
        return -1;
    }

    if (sem_init(&(source->empty_ready), 1, 0) == -1) {
        sem_destroy(&source->data_ready);
        fprintf(stderr, "[source %d]: can't init source semaphore (empty)\n", source->sid);
        return -1;
    }

    if (sdesc->type == STYPE_TCP) {
        pthread_create(&source->thread_id, NULL, source_data_thread_tcp, source);
    }
    else {
        pthread_create(&source->thread_id, NULL, source_data_thread_udp, source);
    }

    *psource = source;

    return source->fd;
}

void source_destroy(struct data_source *source){
    source->exit_flag = 1;
    sleep(5);
    if (source->camera) {
        source->camera->state = STREAM_STOP;
        sem_post(&source->empty_ready);
        gige_camera_destroy(source->camera);
        free(source->camera);
    }
    if (source->packet) {
        free((source->packet)->header);
        free(source->packet);
    }
    close(source->fd);
    source->fd = -1;
    pthread_join(source->thread_id, NULL);
    sem_destroy(&source->empty_ready);
    sem_destroy(&source->data_ready);
    free(source->sdesc);
    if (source->need_to_free) {
        free(source);
    }
}
