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

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include <semaphore.h>
#include <pthread.h>

#include "tcpcapture/cvcamera.h"
#include <dclfilters/cvfilter.h>

#include "tcpcapture/cvcollector.h"
#include <misc/name_generator.h>
#include <storage/storage_interface.h>

#include  <iostream>
#include <storage/Ipc.h>
#include <storage/ObjectDescriptor.h>

using namespace tmk;
using namespace tmk::storage;

#define MAX_PENDING_CONNECT 10

void *cv_collect_data_thread(void *arg){

    struct cv_data_collector *collector = (struct cv_data_collector*)arg;
    int fd = (collector->src).fd;

    auto [segment, clientShmemAlloc] = ipc::openSharedMemory(collector->segment);
    auto clientObj = ipc::findShared<ObjectDescriptor>(segment, collector->data);

    cv::Mat img = cv::Mat::zeros(IMAGE_RAW_HEIGHT, IMAGE_RAW_WIDTH, CV_8UC3);
    size_t imgSize = img.total() * img.elemSize();

    char header[IMAGE_HEADER_SIZE];
    size_t  data_size = 0;
    int frame_id = -1;
    int source_id = 0;
    struct iovec iov[2];

    iov[0].iov_base = (char*)header; //(size_t*)&data_size;
    iov[0].iov_len = IMAGE_HEADER_SIZE;
    iov[1].iov_base = (char*)img.data;
    iov[1].iov_len = IMAGE_RAW_SIZE;

    //int iovcnt = sizeof(iov) / sizeof(struct iovec);

    int bytes = 0;
    size_t recieved_len = 0, remaining_len = iov[0].iov_len;
    int id = 0;

#if 0
    if (!img.isContinuous()) {
        img = img.clone();
    }
#endif

    int header_ok = 0;

    int frames_obj_cnt = 0;
    int frames_empty_cnt = 0;
    int frames_non_cnt = 0;
    int new_object = 0;

    int framesets_cnt = 0;
    //int frames_cnt = 0;

    sem_wait(&collector->empty_ready);
    while (!(collector->exit_flag)) {
        if (remaining_len == 0) {
            //fprintf(stdout, "wtf???\n");
            remaining_len = iov[0].iov_len;
            continue;
        }
#if 0
        if (!header_ok && recieved_len == 0) {
            fprintf(stdout, "1\n");
            bytes = readv(fd, iov, iovcnt);
        }
        else
#endif
        if (!header_ok) {
            //fprintf(stdout, "2\n");
            bytes = read(fd, (char*)iov[0].iov_base + recieved_len, remaining_len);
        }
        else {
            //fprintf(stdout, "3: recv = %zd, rem = %zd\n", recieved_len, remaining_len);
            bytes = read(fd, (char*)iov[1].iov_base + recieved_len, remaining_len);
        }
        if (bytes < 0) {
            //fprintf(stdout, "[tcp_capture_data_process]: error while frame_size read\n");
            collector->exit_flag = 1;
            break;
        }
        else if (bytes == 0) {
            usleep(1000);
            continue;
        }
        else {
            //printf("header=%d bytes=%zd recieved=%zd remining=%zd\n", header_ok, bytes, recieved_len, remaining_len);
            recieved_len += bytes;
            if (!header_ok && recieved_len < iov[0].iov_len) {
                remaining_len -= bytes;
            }
            if (!header_ok && recieved_len == iov[0].iov_len) {
                data_size = *((size_t*)iov[0].iov_base);
                frame_id = *((int*)((char*)iov[0].iov_base + sizeof(size_t)));
                //fprintf(stdout, "[tcp_capture_data_process]: found_image_size = %zd found_frame_id = %d\n", data_size, frame_id);
                if (data_size != DATA_SIZE) {
                    fprintf(stdout, "[collect_data_thread]: strange data size = %zd instead of %zd\n", data_size, (size_t)DATA_SIZE);
                    fflush(stdout);
                }
#if 0
                recieved_len -= iov[0].iov_len;
                remaining_len = iov[1].iov_len - recieved_len;
#else
                recieved_len = 0;
                remaining_len = data_size;
#endif
                header_ok = 1;
            }
            if (header_ok && recieved_len < iov[1].iov_len) {
                remaining_len -= bytes;
            }
            if (header_ok && recieved_len == iov[1].iov_len) {
                if (imgSize != recieved_len) {
                    fprintf(stdout, "[collect_data_thread]: result size = %zd instead of %zd\n", recieved_len, imgSize);
                    fflush(stdout);
                }
//------------------------------------------------------------------------------------
                float radius = findRadius(img);
                //not smart algotithm of object detection
                //fprintf(stdout, "[tcp_capture_data_process]: frames_obj_cnt = %d frames_empty_cnt %d frames_non_cnt %d\n", frames_obj_cnt, frames_empty_cnt, frames_non_cnt);
                if (radius < 135 && new_object == 0) {
                    frames_empty_cnt++;
                    if (frames_obj_cnt >= 16 && frames_empty_cnt >= 8) {
                        new_object = 1;
                        fprintf(stdout, "[collect_data_thread]: object break is detected after %d frames\n", frames_empty_cnt + frames_obj_cnt + frames_non_cnt);
                        fflush(stdout);
//------------------------------------------------------------------------------
                        //TODO: print RESUME about collected object
                        //TODO: for debug check .valid() of first and random frame, may be save it 
                        //TODO: check time for object collect
//------------------------------------------------------------------------------
                        //finalize current full object
                        sem_post(&collector->data_ready);
                        new_object = 0;
                        frames_obj_cnt = 0;
                        frames_non_cnt = 0;
                        frames_empty_cnt = 0;
                        fprintf(stderr, "[collect_data_thread]: exit_flag = %d\n", collector->exit_flag);
                        if (!collector->exit_flag) {
                            sem_wait(&collector->empty_ready);
                        }
//------------------------------------------------------------------------------  
                        // New object ready, start empty                          
                        clientObj = ipc::findShared<ObjectDescriptor>(segment, collector->data);
                        framesets_cnt = clientObj().framesetsCount();
                        fprintf(stdout, "[collect_data_thread]: found %d framesets in the object %s\n", framesets_cnt, collector->data);
                        fflush(stdout);
//------------------------------------------------------------------------------
                    }
                }
                else if (radius >= 150){
                   frames_obj_cnt++;
                   frames_obj_cnt += frames_empty_cnt;
                   frames_empty_cnt = 0;
                   frames_obj_cnt += frames_non_cnt;
                   frames_non_cnt = 0;
                }
                else {
                    frames_non_cnt++;
                }
                //TODO: add frame to object with name from collector->data
                //clientObj = ipc::findShared<ObjectDescriptor>(segment, collector->data);
                //TODO: different collect threads for different sources
                //framesets_cnt = clientObj().framesetsCount();
                //fprintf(stdout, "[collect_data_thread]: found %d framesets in the object %s\n", framesets_cnt, collector->data);
//-------------------------------------------------------------------------------------
                //char name[256];                
                //sprintf(name, "../data/test_%.4d.bmp", id);
                //cv::imwrite(name, img);
                remaining_len = iov[0].iov_len;
                recieved_len = 0;
                header_ok = 0;
                fprintf(stdout, "[collect_data_thread]: Frame %d with frame_id %d is ready, radius = %f\r", id++, frame_id, radius);
                fflush(stdout);
//--------------------------------------------------------------------------------------
                size_t available = storage_interface_add_frame_to_object_with_framesets(collector->segment, collector->data, source_id, frame_id, (void*)(&img));
                (void)available;
                //fprintf(stdout, "[collect_data_thread]: %zd of shared segment available\n", available);
                //fflush(stdout);
//--------------------------------------------------------------------------------------
                //fprintf(stdout, "[collect_data_thread]: Added to %s::%s\n", collector->segment, collector->data);
               //fflush(stdout);
            }
        }
    }
    sem_post(&collector->data_ready);
    fprintf(stdout, "[collect_data_thread]: stop\n");
    fflush(stdout);
    return NULL;
}

struct cv_data_collector *cv_collector_init(char* addr, char *port) {

    struct cv_data_collector *collector = (struct cv_data_collector *)malloc(sizeof(struct cv_data_collector));
    if (!collector) {
        fprintf(stderr, "[collector_init]: can't allocte collector\n");
        fflush(stderr);
        return NULL;
    }

    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int rc, fd;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;     //IPv4 && IPv6
    hints.ai_socktype = SOCK_STREAM; //TCP socket
    hints.ai_flags = 0;
    hints.ai_protocol = 0;           //Any protocol

    rc = getaddrinfo(addr, port, &hints, &result);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return NULL;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) continue;
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1) break;
        close(fd);
    }

    if (rp == NULL) {
        fprintf(stderr, "Could not connect\n");
        return NULL;
    }

    freeaddrinfo(result);

    strcpy((collector->src).addr, addr);
    strcpy((collector->src).port, port);
    (collector->src).fd = fd;
    collector->exit_flag = 0;

    if (sem_init(&(collector->data_ready), 1, 0) == -1) {
        fprintf(stderr, "[cvcollector]: can't init object collector semaphore\n");
        return NULL;
    }

    if (sem_init(&(collector->empty_ready), 1, 0) == -1) {
        sem_destroy(&collector->data_ready);
        fprintf(stderr, "[cvcollector]: can't init object collector semaphore\n");
        return NULL;
    }

    pthread_create(&collector->thread_id, NULL, cv_collect_data_thread, collector);

    return collector;
}

void cv_collector_destroy(struct cv_data_collector *collector){
    collector->exit_flag = 1;
    pthread_join(collector->thread_id, NULL);
    close((collector->src).fd);
    sleep(5);
    sem_destroy(&collector->empty_ready);
    sem_destroy(&collector->data_ready);
    free(collector);
}
