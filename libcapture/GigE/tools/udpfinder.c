#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//open
#include <sys/stat.h>
#include <fcntl.h>
//write/close/sleep
#include <unistd.h>

#include "udpcapture/device.h"

void new_device_callback(void *internal_data, struct device *dev) {
    (void)internal_data;
    fprintf(stdout, "I'm in the new device callback\n");
    fflush(stdout);
    device_print(dev);
}

int main(int argc, char *argv[]) {
    fprintf(stdout, "Utility for find gvcp devices on all interfaces with broadcast\n");
    fprintf(stdout, "Usage: %s [ip]\n", argv[0]);
    fprintf(stdout, "Example: %s 10.0.0.2 -- find device on 10.0.0.2\n", argv[0]);
    fprintf(stdout, "         %s -- find all devices on all interfaces\n", argv[0]);

    if (argc == 1) {
        devices_discovery(NULL, NULL);
    }
    else {
        for (int i = 1; i < argc; i++) {
            struct device *dev = device_find(argv[i]);
            if (!dev) {
                fprintf(stdout, "Device is null\n");
                fflush(stdout);
                return 0;
            }
            new_device_callback(NULL, dev);
            free(dev);
        }
    }
    return 0;
}




