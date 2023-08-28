#include <stdio.h>
#include <unistd.h>

#include <misc/name_generator.h>
#include <pipeline/pipeline_client.h>

void first_action(void *internal_data, void *data, int *status) {
    (void)internal_data;
    char *test_data = (char *)data;
    generate_new_object_name(test_data);
    sleep(1);
    fprintf(stdout, "[first_action]: %s created\n", test_data);
    fflush(stdout);
    *status = 0;
}

int main(int argc, char *argv[]) {
    fprintf(stdout, "Binary %s is module of pipeline with the custom first action\n", argv[0]);
    if (argc > 1) { 
        fprintf(stdout, "Usage: %s\n", argv[0]);
        fflush(stdout);
    }
    pipeline_process(NULL, first_action);
}

