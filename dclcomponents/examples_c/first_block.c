#include <stdio.h>

#include <processor/dclprocessor.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <misc/log.h>
#include <misc/name_generator.h>
#include <storage/storage_interface.h>

#include <dclfilters/cvtools.h>

#include <description/description_json.h>
#include <description/collector_description.h>
#include <collector/collector.h>

void *restore_image(struct source_description *sdesc, void *frame_data) {
    void *img_addr;
    size_t rows = sdesc->height;
    size_t cols = sdesc->width;
    int channels = sdesc->channels;
    restore_CV_8U_m(&img_addr, rows, cols, channels, frame_data);
    return img_addr;
}

void save_image(const char *name, struct source_description *sdesc, void *frame_data) {
    save_cv_image(name, sdesc->height, sdesc->width, sdesc->channels, frame_data);
}

void add_frame_callback(void *internal_data, int source_id, int frame_id, void *frame_data) {
    struct data_collector *collector = (struct data_collector *)internal_data;
    //fprintf(stdout, "[%s]: Add frame from collector=%p\n", __FUNCTION__, (void*)collector);
    //fflush(stdout);
    if (collector) {
        struct source_description *sdesc = (collector->sources[source_id])->sdesc;
        void *img_addr = restore_image(sdesc, frame_data);
        size_t available = storage_interface_add_frame_to_object_with_framesets(collector->boost_path, (collector->sources[source_id])->data, source_id, frame_id, img_addr);
        delete_m(&img_addr);
        (void)available;
        //fprintf(stdout, "[%s]: %zd of shared segment available\n", __FUNCTION__, available);
        //fflush(stdout);
    }
}

void save_frame_callback(void *internal_data, int source_id, int frame_id, void *frame_data) {
    (void)frame_id;
    struct data_collector *collector = (struct data_collector *)internal_data;
    //fprintf(stdout, "[%s]: collector=%llx\n", __FUNCTION__, collector);
    //fflush(stdout);
    if (collector) {
        struct source_description *sdesc = (collector->sources[source_id])->sdesc;
        char *output_path = sdesc->output_path;
        char postfix[DT_POSTFIX_SIZE];
        char name[2048];
        generate_dt_postfix(postfix);
        sprintf(name, "%s/frame_%d_%s.bmp", output_path, source_id, postfix);
        save_image(name, sdesc, frame_data);
    }
}

int main(int argc, char *argv[]) {
    /*if (argc < 2) {
        fprintf(stdout, "Usage: %s init_argv\n", argv[0]);
        fprintf(stdout, "Examples:\n");
        fprintf(stdout, "\t%s collector\n", argv[0]);
        fprintf(stdout, "\t%s server:4444 pipeline:default_collector:tcp_default:camera\n", argv[0]);
        fflush(stdout);
    }*/

    (void)argc;
    (void)argv;

    struct dclprocessor *proc = dclprocessor_get(1); //take information from ENV
    //collector_init(proc->collector);
    struct data_collector *collector = (struct data_collector *)proc->collector;
                for (int i = 0; i < collector->sources_cnt; i++) {
                    (collector->sources[i])->internal_data = (void *)collector;
                    (collector->sources[i])->object_detection_callback = NULL;
                    //TODO: should work only for one source now
                    //(collector->sources[i])->object_detection_callback = object_detection_callback;
                    (collector->sources[i])->add_frame_callback = add_frame_callback;
                    (collector->sources[i])->save_frame_callback = NULL;
                    //(collector->sources[i])->save_frame_callback = save_frame_callback;
                }

    attach_to_pipeline(proc, NULL, NULL);

    return 0;
}
