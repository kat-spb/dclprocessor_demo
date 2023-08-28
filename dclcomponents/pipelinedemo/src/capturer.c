#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pipeline/pipeline_client.h>

void pipeline_action(void *internal_data, void *data, int *status) {
    (void)internal_data;
    char s[80];
    static int data_cnt = 0;
    fprintf(stdout, "[first, pipeline_action]: Input data: ");
    fflush(stdout);
    scanf("%s", s);
    fflush(stdout);
    sprintf(data, "%d%s", data_cnt++, s);
    *status = 0;
}

int main(){
    pipeline_process(NULL, pipeline_action);
    return 0;
}

