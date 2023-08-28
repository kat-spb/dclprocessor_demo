#include <stdio.h>
#include <unistd.h>
#include <pipeline/pipeline_client.h>

void last_action(void *internal_data, void *data, int *status) {
    (void)internal_data;
    char *test_data = (char *)data;
    static int data_id = 0;
    fprintf(stdout, "[last_action, #%d]: %s\n", data_id++, test_data);
    fflush(stdout);
    *status = 0;
}

int main(int argc, char *argv[]) {
    fprintf(stdout, "Binary %s is module of pipeline with the custom first action\n", argv[0]);
    if (argc > 1) { 
        fprintf(stdout, "Usage: %s\n", argv[0]);
        fflush(stdout);
    }

    pipeline_process(NULL, last_action);
}

