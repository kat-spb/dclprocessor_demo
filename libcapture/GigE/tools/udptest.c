#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//open
#include <sys/stat.h>
#include <fcntl.h>
//write/close/sleep
#include <unistd.h>

#include "udpcapture/device.h"
#include "udpcapture/gige_camera.h"

#if 0
#include "dclprocessor/iframe.h"
void new_frame_action(void *buffer, unsigned int buffer_size, void *extended_data) {
    //this action is save-callback for buffer
    int frame_id = *(int*)extended_data;
    char fname[50];
    sprintf(fname, "../data/out_%d.raw", frame_id);
    fprintf(stdout, "[save_frame_callback]: out frame to %s as binary\n", fname);
    int out_fd;
    out_fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (out_fd > 0) {
        write(out_fd, buffer, buffer_size);
        close(out_fd);
    }
}
#endif

void new_device_callback(void *internal_data, struct device *dev) {
    //(void)internal_data;
    int duration = *(int*)internal_data;
    fprintf(stdout, "[new_device_callback]: I'm in the new device callback\n");
    fflush(stdout);
    device_print(dev);
#if 0
    struct gige_camera *cam = gige_camera_init(dev, new_frame_action);
#else
    struct gige_camera *cam = gige_camera_init(dev, NULL);
#endif
    if (cam == NULL) {
        fprintf(stdout, "[new_device_callback]: cam is NULL?\n");
        fflush(stdout);
        return;
    }
    gige_camera_test(cam, duration);
    gige_camera_destroy(cam);
    free(cam);
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 4) {
        fprintf(stdout, "Utility for test ONE gvcp device\n");
        fprintf(stdout, "Usage: %s ip [duration]\n", argv[0]);
        fprintf(stdout, "Example: %s 10.0.0.2\n", argv[0]);
        fprintf(stdout, "Example: %s 10.0.0.2 4  --- camera test for 4s\n", argv[0]);
        fprintf(stdout, "Example: %s 10.0.0.2 0  --- camera test without limit\n", argv[0]);
        fprintf(stdout, "Example: %s 10.0.0.2 -1 --- no camera test, only device callback\n", argv[0]);
        fflush(stdout);
    }
    else {
        struct device *dev = device_find(argv[1]);
        if (!dev) {
            fprintf(stdout, "Device is null\n");
            fflush(stdout);
            return 0;
        }
        int duration = 4; //default: camera test for 4s
        if (argc == 3) {
            duration = atoi(argv[2]);
        }
        new_device_callback((void*)&duration, dev);
        free(dev);
    }
    return 0;
}



