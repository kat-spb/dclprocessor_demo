#include <stdio.h>
#include <string.h>

#include <description/pipeline_description.h>
#include <pipeline/pipeline.h>

#define MAX_CMD_LEN 80

int main(int argc, char *argv[]) {
    struct pipeline *pl = NULL;
    struct pipeline_description *pdesc = NULL;

    fprintf(stdout, "Binary %s is start pipeline as server\n", argv[0]);
    if (argc != 2) { 
        fprintf(stdout, "Usage: %s [config_string or config_fname]\n", argv[0]);
        fprintf(stdout, "Examples: \n");
        fprintf(stdout, "\t %s minimal\t-- [na] pipeline with only initial and final thread, do nothing\n", argv[0]);
        fprintf(stdout, "\t %s demo\t-- [na] pipeline with 2 modules (first and middle) and final thread, do nothing\n", argv[0]);
        fprintf(stdout, "\t %s path/config_with_pipeline.json\t-- [ok] pipeline from config\n", argv[0]);
        fflush(stdout);
        return 0;
    }

    int is_started =  0;
    while (1) {
        if (argc == 2) {
            load_pipeline_configuration_from_file(argv[1], &pdesc);
            is_started = !pipeline_init(&pl, pdesc);
            argc = 1;
        }
#if 0
        char cmd[MAX_CMD_LEN];
        fprintf(stdout, "Input command for %s:", argv[0]);
        fflush(stdout);
        scanf("%s", cmd);
        if (strcasecmp(cmd, "stop") == 0) {
            //TODO: use pipeline_stop(pl); -- not implemented yet
            if (is_started) {
                pipeline_destroy(pl);
                pl = NULL;
                is_started = 0;
            }
            else {
                fprintf(stdout, "[%s]: pipeline already stopped\n", argv[0]);
                fflush(stdout);
            }
            continue;
        }
        if (strcasecmp(cmd, "start") == 0) {
            //TODO: use pipeline_start(pl); -- not implemented yet
            if (!is_started) {
                is_started = !pipeline_init(&pl, pdesc);
            }
            else {
                fprintf(stdout, "[%s]: pipeline already started\n", argv[0]);
                fflush(stdout);
            }
            continue;
        }
        if (strcasecmp(cmd, "config") == 0) {
            //TODO: use pipeline_start(pl); -- not implemented yet
            scanf("%s", cmd);
            if (!is_started) {
                create_pipeline_description(cmd, &pdesc);
            }
            else {
                fprintf(stdout, "[%s]: pipeline started, please stop current first\n", argv[0]);
                fflush(stdout);
            }
            continue;
        }
        if (strcasecmp(cmd, "quit") == 0) {
            if (is_started) {
                pipeline_destroy(pl);
                pl = NULL;
                is_started = 0;
            }
            free_pipeline_description(&pdesc);
            break;
        }
#else
        (void)is_started;
#endif
    }

    return 0;
}

