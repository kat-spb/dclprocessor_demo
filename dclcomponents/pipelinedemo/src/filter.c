#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pipeline/pipeline_client.h>

void pipeline_action(void *internal_data, void *data, int *status) {
    (void)internal_data;
    fprintf(stdout, "[middle, pipeline_action]: Get: %s\n", (char*)data);
    fflush(stdout);
    *status = (strchr(data, '*') == NULL);
}

int main(){
    pipeline_process(NULL, pipeline_action);
    return 0;
}

