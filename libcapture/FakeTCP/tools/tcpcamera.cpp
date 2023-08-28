#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "tcpcapture/cvgenerator.h"

int main(int argc, char *argv[]){
    if (argc < 2) {
        fprintf(stderr, "Usage: %s port is_daemon = 0\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    generator_init(argv[1]);
    return 0;
}



