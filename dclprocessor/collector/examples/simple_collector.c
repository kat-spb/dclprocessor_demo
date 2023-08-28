#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <misc/log.h>
#include <misc/name_generator.h>
#include <storage/storage_interface.h>

#include <dclfilters/cvtools.h>

#include <description/collector_description.h>
#include <collector/collector.h>

void create_test_collector_description(int sources_cnt, struct collector_description **pdesc) {
    struct collector_description *desc = *pdesc;
    if (!desc) desc = (struct collector_description *)malloc(sizeof(struct collector_description));
    memset(desc, 0, sizeof(struct collector_description));
    desc->sources_cnt = sources_cnt;
    if (!desc->sdescs) {
        desc->sdescs = (struct source_description **)malloc(desc->sources_cnt * sizeof(struct source_description *));
        memset(desc->sdescs, 0, desc->sources_cnt * sizeof(struct source_description *));
        for (int i = 0; i < desc->sources_cnt; i++) {
            char port[11];
            sprintf(port, "%d", 9000 + i * 10);
            create_tcp_source_description("localhost", port, &(desc->sdescs[i]));
        }
    }
    desc->use_boost_shmem = 1;
    if (desc->use_boost_shmem) {
        strcpy(desc->boost_path, "boostsegment");
        //desc->boost_size = 5000000000; //~5Gb
        desc->boost_size = calculate_boostmem_size(desc->sources_cnt, 2048 * 2448 * 1, 1000);
    }
    *pdesc = desc;
}

void create_demo_collector_description(struct collector_description **pdesc) {
    struct collector_description *desc = *pdesc;
    if (!desc) desc = (struct collector_description *)malloc(sizeof(struct collector_description));
    memset(desc, 0, sizeof(struct collector_description));
    desc->sources_cnt = 2;
    if (!desc->sdescs) {
        desc->sdescs = (struct source_description **)malloc(desc->sources_cnt*sizeof(struct source_description *));
        memset(desc->sdescs, 0, desc->sources_cnt * sizeof(struct source_description *));
        create_gige_source_description("10.0.0.2", &(desc->sdescs[0])); //flir-color2
        //create_gige_source_description("169.254.2.134", &(desc->sdescs[1])); //flir-color
        //create_gige_source_description("169.254.0.1", &(desc->sdescs[1])); //lucid polar
        create_gige_source_description("10.0.0.3", &(desc->sdescs[1])); //lucid polar
    }
    desc->use_boost_shmem = 1;
    if (desc->use_boost_shmem) {
        strcpy(desc->boost_path, "boostsegment");
        desc->boost_size = 30000000000; //~30Gb
        //desc->boost_size = 58048974720; //~60Gb
    }
    *pdesc = desc;
}

//this action took full object name and prepare new name for new empty object
//last full obj_name returns in data and puts with this action to collector objects ring
void collector_action(struct data_collector *collector, void *data, int *status) {
    if (!collector) {
        return;
    }
    char obj_name[256];
    if (collector->exit_flag) {
        sem_post(&(collector->empty_ready));
        *status = 0;
        return;
    }
    fprintf(stdout, "[%s]: I'm in collector action before wait(collector:data)\n", __FUNCTION__);
    fflush(stdout);
    sem_wait(&(collector->data_ready));
    static int data_id = 0;
    fprintf(stdout, "[%s]: Object #%d with name %s is ready\n", __FUNCTION__, data_id++, collector->data);
    fflush(stdout);
    //prepare place for current ready collector->data(=full_object_name) in the collector->data_ring
    data = (char*)collector->data_memory + collector->dataoffset;
    //copy string to ring memory
    strcpy((char*)data, collector->data);
    ring_buffer_push(collector->data_ring, collector->dataoffset);
    fprintf(stdout, "[%s]: Dataoffset = %zu pushed to data_ring, data = %s(%p)\n", __FUNCTION__, collector->dataoffset, (char*)data, (void*)data);
    fflush(stdout);
    collector->dataoffset = (collector->dataoffset + collector->chunk_size) % (collector->chunk_cnt * collector->chunk_size);;
    //generate name for next object
    generate_new_object_name(obj_name);
    storage_interface_new_object_with_framesets(collector->boost_path, obj_name, collector->sources_cnt);
    fprintf(stdout, "[%s]: %s created\n", __FUNCTION__, obj_name);
    fflush(stdout);
    fprintf(stdout, "[%s]: Set new generated name %s to collector->data\n", __FUNCTION__, obj_name);
    fflush(stdout);
    strcpy(collector->data, obj_name); //active collector object for add frame
    fprintf(stdout, "[%s]: I'm  in first action before post(collector:empty)\n", __FUNCTION__);
    fflush(stdout);
    sem_post(&(collector->empty_ready));
    fprintf(stdout, "[%s]: I'm  in first action after post(collector:empty)\n", __FUNCTION__);
    fflush(stdout);
    *status = 0;
}

void after_collector_action(struct data_collector *collector, void *data, int *status) {
    if (!collector) {
        return;
    }
    size_t dataoffset = ring_buffer_pop(collector->data_ring);
    char *obj_name = (char*)collector->data_memory + dataoffset;
    fprintf(stdout, "[%s]: %zu poped from data_ring, data = %s[%p]\n", __FUNCTION__, dataoffset, obj_name,(void*)obj_name);
    fflush(stdout);
    if (strlen(obj_name) > 0) {
        strcpy((char*)data, obj_name);
        *status = 0;
    }
    else {
        *status = 1;
    }
}

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
    fprintf(stdout, "Binary %s is start collector\n", argv[0]);
    if (argc != 1) { 
#if 0
        fprintf(stdout, "Usage: %s [tcp | udp | config_string or config_fname] [sources_cnt] [sources ...]\n", argv[0]);
        fprintf(stdout, "Examples: \n");
        fprintf(stdout, "\t %s tcp 0\t-- predefined collector for 1 fake tcp source on localhost:9000\n", argv[0]);
        fprintf(stdout, "\t %s udp 0\t-- predefined collector for 1 fake udp source on localhost\n", argv[0]);
        fprintf(stdout, "\t %s udp 1 127.0.0.1\t-- collector for 1 aravis-fake udp source on localhost", argv[0]);
        fprintf(stdout, "\t %s udp 1 10.0.0.2\t-- collector for 1 real udp source on 10.0.0.2:9000", argv[0]);
        fprintf(stdout, "\t %s tcp 3 9000 9010 9020\t-- pipeline with one final thread, do nothing\n", argv[0]);
        fprintf(stdout, "\t %s udp 2 10.0.0.2 10.0.0.3\t-- pipeline with 2 modules (first and middle) and final thread, do nothing\n", argv[0]);
        fprintf(stdout, "\t %s data/collector_example.json\t-- collector from config\n", argv[0]);
        fflush(stdout);
#endif
        return 0;
    }

    struct collector_description *desc = NULL;
#if 1 //with n tcp-fake cameras
    create_test_collector_description(1, &desc);
#else //with to sources at tmk-serv
    create_demo_collector_description(&desc); //-- 2 cameras, problems after 10-13 objects full-size
#endif

    struct data_collector *collector = NULL;
    collector = collector_init(desc);
    if (!collector) {
        fprintf(stdout, "[%s]: collector init return NULL, may be camera isn't ready. Please, try again or work without stream.\n", argv[0]);
        fflush(stdout);
    }

    for (int i = 0; i < collector->sources_cnt; i++) {
        (collector->sources[i])->internal_data = (void *)collector;
        (collector->sources[i])->object_detection_callback = NULL;
        //TODO: should work only for one source now
        //(collector->sources[i])->object_detection_callback = object_detection_callback;
        (collector->sources[i])->add_frame_callback = add_frame_callback;
        //(collector->sources[i])->save_frame_callback = NULL;
        (collector->sources[i])->save_frame_callback = save_frame_callback;
    }

    generate_new_object_name(collector->data);
    if (collector->use_boost_shmem) {
        //if we want make new version of boost shmem for new start of first module
        storage_interface_destroy(collector->boost_path);
        size_t realsize = storage_interface_init(collector->boost_path, collector->boost_size);
        if (realsize != collector->boost_size) {
            //TODO: this difference always -- what the reason? (may be allocators or some else ...)
            fprintf(stderr, "[pipeline_init]: asked about %zd, but real allocated %zd bytes\n", collector->boost_size, realsize);
            fflush(stderr);
        }
        else {
             fprintf(stdout, "[pipeline_init]: real allocated %zd bytes\n", realsize);
             fflush(stdout);
        }
        storage_interface_new_object_with_framesets(collector->boost_path, collector->data, collector->sources_cnt);
    }

    char data[256];
    data[0] = '\0';
    int status;

    fprintf(stdout, "[%s]: try process in cycle (without pipeline)\n", __FUNCTION__);
    fflush(stdout);

    while (1) {
        collector_action(collector, (void *)data, &status);
        after_collector_action(collector, (void *)data, &status);
        storage_interface_analyze_object_with_framesets(collector->boost_path, data);
        storage_interface_free_object(collector->boost_path, data);
    }

    storage_interface_destroy(collector->boost_path);

    return 0;
}
