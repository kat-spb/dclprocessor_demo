#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <misc/log.h>
#include <misc/file.h>
#include <description/collector_description.h>
#include <description/source_description.h>

struct collector_description *collector_description_create() {
    struct collector_description *desc = (struct collector_description *)malloc(sizeof(struct collector_description));
    memset(desc, 0, sizeof(struct collector_description));
    return desc;
}

int parse_sources_json(struct json_object *sources_json, struct collector_description *desc) {
    if (!sources_json) return 0;
    if (json_object_get_type(sources_json) == json_type_array) {
        struct json_object *source_json;
        size_t i, n_sources = json_object_array_length(sources_json);
#if 0
        fprintf(stdout, "\tFound %lu sources\n", n_sources);
        for (i = 0; i < n_sources; i++) {
            source_json = json_object_array_get_idx(sources_json, i);
            fprintf(stdout, "[%lu]:\t", i);
            parse_source_json(source_json);
        }
#else
        desc->sources_cnt = n_sources;
        desc->sinfos = (struct source_info **)malloc(desc->sources_cnt * sizeof(struct source_info *));
        memset(desc->sinfos, 0, desc->sources_cnt * sizeof(struct source_info *));
        //TODO: temporary -- delete this part
        desc->sdescs = (struct source_description **)malloc(desc->sources_cnt * sizeof(struct source_description *));
        memset(desc->sdescs, 0, desc->sources_cnt * sizeof(struct source_description *));
        for (i = 0; i < n_sources; i++) {
            source_json = json_object_array_get_idx(sources_json, i);
            parse_source_json(source_json, &(desc->sinfos[i]));
            //TODO: temporary -- delete this part
            desc->sdescs[i] = source_description_create();
            update_source_description(desc->sdescs[i], desc->sinfos[i]);
        }
#endif
        return n_sources;
    }
    return 0;
}

void parse_collector_json(struct json_object *jobj, struct collector_description *desc) {
    struct json_object *current;
    struct json_object *sources_json;
    json_object_object_get_ex(jobj, "boost_path", &current);
    if (current) {
        strcpy(desc->boost_path, json_object_get_string(current));
        if (strlen(desc->boost_path) > 1) desc->use_boost_shmem = 1;
        //TODO: this fiels is calculatable
        json_object_object_get_ex(jobj, "boost_size", &current);
        //TODO: this is problem place -- 5Gb more then int
        if (current) desc->boost_size = (size_t)json_object_get_int(current);
    }
    json_object_object_get_ex(jobj, "sources", &sources_json);
    if (sources_json) desc->sources_cnt = parse_sources_json(sources_json, desc);
    //json_object_object_get_ex(jobj, "max_frames_count", &current);
    //if (current) fprintf(stdout, "\tmax_frames_count: %d\n", json_object_get_int(current));
    //TODO: strange, i'm not sure
    //json_object_object_get_ex(jobj, "object_data_size", &current);
    //if (current) fprintf(stdout, "\tobject_data_size: %d\n", json_object_get_int(current));
    json_object_object_get_ex(jobj, "output_path", &current);
    if (current) desc->output_path = strdup(json_object_get_string(current));
}

void load_collector_configuration(const char *fname, struct collector_description **p_desc) {
    if (!p_desc) return;
    if (*p_desc) free(*p_desc);
    struct collector_description *desc = collector_description_create();
    LOG_FD_INFO("Created desc = %p, try to load '%s'\n", desc, fname);
    if (is_exist(fname)) {
        struct json_object *jobj = json_object_from_file(fname);
        parse_collector_json(jobj, desc);
        out_collector_json(jobj);
        LOG_FD_INFO("Created desc = %p jobj = %p\n", desc, jobj);
    }
    else {
        LOG_FD_INFO("Collector with predefined configuration is not supported in this version\n");
        free(desc);
        desc = NULL;
    }
    *p_desc = desc;
}

void out_frame_json(struct json_object *frame_json) {
    struct json_object *current;
    if (!frame_json) return;
    json_object_object_get_ex(frame_json, "width", &current);
    if (current) fprintf(stdout, "\t\twidth: %d\n", json_object_get_int(current));
    json_object_object_get_ex(frame_json, "height", &current);
    if (current) fprintf(stdout, "\t\theight: %d\n", json_object_get_int(current));
    json_object_object_get_ex(frame_json, "format", &current);
    if (current) fprintf(stdout, "\t\tformat: %d\n", json_object_get_int(current));
    json_object_object_get_ex(frame_json, "source_config", &current);
    if (current) fprintf(stdout, "\t\tsource_config: %s\n", json_object_get_string(current));
}

void out_source_json(struct json_object *source_json) {
    struct json_object *current, *frame_json;
    if (!source_json) return;
    json_object_object_get_ex(source_json, "type", &current);
    if (current) fprintf(stdout, "\ttype: %d\n", json_object_get_int(current));
    json_object_object_get_ex(source_json, "id_string", &current);
    if (current) fprintf(stdout, "\tid_string: %s\n", json_object_get_string(current));
    json_object_object_get_ex(source_json, "host", &current);
    if (current) fprintf(stdout, "\thost: %s\n", json_object_get_string(current));
    json_object_object_get_ex(source_json, "port", &current);
    if (current) fprintf(stdout, "\tport: %s\n", json_object_get_string(current));
    json_object_object_get_ex(source_json, "frame", &frame_json);
    if (current) fprintf(stdout, "\tframe:\n");
    if (frame_json) out_frame_json(frame_json);
    json_object_object_get_ex(source_json, "source_config", &current);
    if (current) fprintf(stdout, "\tsource_config: %s\n", json_object_get_string(current));
}

void out_sources_json(struct json_object *sources_json) {
    struct json_object *source_json;
    if (!sources_json) return;
    if (json_object_get_type(sources_json) == json_type_array) {
        size_t i, n_sources = json_object_array_length(sources_json);
        fprintf(stdout, "\tFound %lu sources\n", n_sources);
        for (i = 0; i < n_sources; i++) {
            source_json = json_object_array_get_idx(sources_json, i);
            fprintf(stdout, "[%lu]:\t", i);
            out_source_json(source_json);
        }
    }
}

void out_collector_json(struct json_object *collector_json) {
    struct json_object *current;
    struct json_object *sources_json;
    LOG_FD_INFO("I'm here\n");
    json_object_object_get_ex(collector_json, "boost_path", &current);
    if (current) fprintf(stdout, "\tboost_path: %s\n", json_object_get_string(current));
    json_object_object_get_ex(collector_json, "boost_size", &current);
    if (current) fprintf(stdout, "\tboost_size: %d\n", json_object_get_int(current));
    json_object_object_get_ex(collector_json, "sources", &sources_json);
    if (sources_json) out_sources_json(sources_json);
    json_object_object_get_ex(collector_json, "max_frames_count", &current);
    if (current) fprintf(stdout, "\tmax_frames_count: %d\n", json_object_get_int(current));
    json_object_object_get_ex(collector_json, "object_data_size", &current);
    if (current) fprintf(stdout, "\tobject_data_size: %d\n", json_object_get_int(current));
    json_object_object_get_ex(collector_json, "output_path", &current);
    if (current) fprintf(stdout, "\toutput_path: %s\n", json_object_get_string(current));
}


void out_collector_description(struct collector_description *desc) {
    fprintf(stdout, "Collector description:\n");
    fprintf(stdout, "\tnumber of sources = %d\n", desc->sources_cnt);
    for (int i = 0; i < desc->sources_cnt; i++) {
       if (desc->sinfos) out_source_info(desc->sinfos[i]);
       if (desc->sdescs) out_source_description(desc->sdescs[i]);
    }
    if (desc->use_boost_shmem) {
        fprintf(stdout, "Use boost shared memory:\tYES\n");
        fprintf(stdout, "\tboost region path = \"%s\"\n", desc->boost_path);
        fprintf(stdout, "\tboost region size = \"%zd\"\n", desc->boost_size);
    }
    else {
        fprintf(stdout, "Use boost shared memory:\tNO\n");
    }
    fflush(stdout);
}

void collector_description_destroy(struct collector_description **p_desc) {
    if (!p_desc || *p_desc == NULL) return;
    struct collector_description *desc = *p_desc;
    for (int i = 0; i < desc->sources_cnt; i++) {
        if (desc->sinfos) source_info_destroy(&desc->sinfos[i]);
        if (desc->sdescs) source_description_destroy(&desc->sdescs[i]);
    }
    free(desc->sdescs);
    if (desc->output_path) free(desc->output_path);
    free(desc);
    *p_desc = NULL;
}

