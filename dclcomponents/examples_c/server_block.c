#include <stdio.h>
#include <unistd.h>

#include <description/dclprocessor_description.h>
#include <processor/dclprocessor.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stdout, "Usage: %s dclprocessor_config\n", argv[0]);
        fprintf(stdout, "Examples:\n");
        fprintf(stdout, "\t%s ../config/dclprocessor_tcp.json\n", argv[0]);
        fflush(stdout);
    }

    struct dclprocessor *dclproc = NULL;

    struct dclprocessor_description *desc = NULL;
    load_dclprocessor_description(argv[1], &desc);

    int rc = dclprocessor_init(&dclproc, desc); //state_flags = DP_SELF_CREATE
    if (rc != 0) {
        fprintf(stderr, "[%s]: CRITICAL ERROR -- couldn't init dclprocessor\n", argv[0]);
        fflush(stderr);
        dclprocessor_description_destroy(&desc);
        return 0;
    }
/*
    if (dproc->messenger_type == DP_MSG_PROXY) {
        struct proxy *px = NULL;
        proxy_init(init_argv);
        dproc->messenger = (void*)px;
        dproc->messenger_method = processor_proxy_thread;
        dproc->state_flags |= DPF_MSG_INIT;
    }
    else (dproc->messenger_type == DP_MSG_SERVER) {
        struct server *srv = NULL;
        server_init(init_argv);
        dproc->messenger = (void*)srv;
        dproc->messenger_method = processor_server_thread;
        dproc->state_flags |= DPF_MSG_INIT;
    }

    //if (dproc->state_)
*/

    //TODO: work with status
    while (1) {
        //fprintf(stdout, "[%s]: exit_flag = %d\n", argv[0], dclproc->pipeline->exit_flag);
        //fflush(stdout);
        if (dclproc->pipeline->exit_flag) {
            break;
        }
        sleep(1);
    }

    dclprocessor_description_destroy(&desc);

    return 0;
}
