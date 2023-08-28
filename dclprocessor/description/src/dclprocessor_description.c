#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <misc/log.h>
#include <description/dclprocessor_description.h>
#include <description/description_json.h>

struct dclprocessor_description *dclprocessor_description_create() {
    struct dclprocessor_description *desc = (struct dclprocessor_description *)malloc(sizeof(struct dclprocessor_description));
    if (!desc) {
        LOG_FD_ERROR("can't allocte dclprocessor\n");
        return NULL;
    }
    memset(desc, 0, sizeof(struct dclprocessor_description));
    LOG_FD_INFO("New dclprocessor description CREATED, &desc = %p\n", desc);
    return desc;
}

int parse_dclprocessor_json(struct json_object *jobj, struct dclprocessor_description *desc) {
    if (!desc) {
        LOG_FD_ERROR("desc is NULL\n");
        return -1;
    }

    //print_object_json(jobj, "System json");

/*
    const char *key_c = "dclprocessor_config";
    const char *key = "dclprocessor";
    struct json_object *dclprocessor_config_json;
    struct json_object *dclprocessor_json;
    if (find_in_json(jobj, key_c) != NULL) {
        json_object_object_get_ex(jobj, key_c, &dclprocessor_config_json);
        LOG_FD_INFO("Found %s config_file: %s\n", key, json_object_get_string(dclprocessor_config_json));
    }
    if (find_in_json(jobj, key) != NULL) {
        json_object_object_get_ex(jobj, key, &dclprocessor_json);
        if (dclprocessor_json == NULL && dclprocessor_config_json != NULL) {
            dclprocessor_json = json_object_from_file(json_object_get_string(dclprocessor_config_json));
        }
    }
    if (!dclprocessor_json) {
        LOG_FD_ERROR("Object doesn't contain pipeline description\n");
        return -1;
    }
*/
    struct json_object *current;
    json_object_object_get_ex(jobj, "use_shmem", &current);
    if (current) desc->use_shmem = json_object_get_int(current);
    json_object_object_get_ex(jobj, "shmem_segment_name", &current);
    if (current) desc->shmpath = strdup(json_object_get_string(current));
    //TODO: calculate ???
    json_object_object_get_ex(jobj, "shmem_segment_size", &current);
    if (current) desc->shmsize = json_object_get_int(current);

    struct json_object *pipeline_jobj = get_pipeline_json(jobj);
    //print_object_json(pipeline_jobj, "Pipeline json");
    desc->pipeline_desc = pipeline_description_create();
    parse_pipeline_json(pipeline_jobj, desc->pipeline_desc);

    struct json_object *collector_jobj = get_collector_json(jobj);
    //print_object_json(collector_jobj, "Collector json");
    desc->collector_desc = collector_description_create();
    parse_collector_json(collector_jobj, desc->collector_desc);

    return 0;
}

void load_dclprocessor_description(char *fname, struct dclprocessor_description **p_desc) {
    if (!p_desc) return;
    struct dclprocessor_description *desc = dclprocessor_description_create();

    struct json_object *jobj = load_object_json(fname);
    struct json_object *dclprocessor_jobj = get_dclprocessor_json(jobj);
    parse_dclprocessor_json(dclprocessor_jobj, desc);
    out_dclprocessor_description(desc);

    struct json_object *system_jobj = construct_full_object(jobj);
    desc->full_json_size = write_object_to_buffer(system_jobj, &(desc->full_json));

    *p_desc = desc;
}

void load_dclprocessor_description_from_buffer(char *buffer, size_t buffer_size, struct dclprocessor_description **p_desc) {
    if (!p_desc) return;
    struct dclprocessor_description *desc = dclprocessor_description_create();
    size_t bytes = 0;
    struct json_object *system_jobj = read_object_from_buffer(buffer, &bytes);
    if (bytes != buffer_size) {
        //TODO: english in message
        LOG_FD_ERROR("value of buffer size isn't that waitting\n");
    }
    struct json_object *dclprocessor_jobj = get_dclprocessor_json(system_jobj);
    parse_dclprocessor_json(dclprocessor_jobj, desc);
    //out_dclprocessor_description(desc);

    desc->full_json_size = buffer_size;
    desc->full_json = (char*)malloc(buffer_size * sizeof(char));
    memcpy(desc->full_json, buffer, buffer_size);

    *p_desc = desc;
}

void save_dclprocessor_description(char *fname, struct dclprocessor_description *desc) {
    (void)fname;
    (void)desc;
}

void out_dclprocessor_description(struct dclprocessor_description *desc) {
    LOG_FD_INFO("===== Out description =====\n");
    fprintf(stdout, "Dclprocessor description:\n");
    if (desc->use_shmem && desc->shmpath) {
        fprintf(stdout, "\twith shared memory in '%s' with size '%zd'\n", desc->shmpath, desc->shmsize);
    }
    else {
        fprintf(stdout, "\twithout shared memory\n");
    }
    out_pipeline_description(desc->pipeline_desc);
    out_collector_description(desc->collector_desc);
    LOG_FD_INFO("===========================\n");
    fflush(stdout);
}

void dclprocessor_description_destroy(struct dclprocessor_description **p_desc) {
    if (p_desc == NULL || *p_desc == NULL ) return;
    struct dclprocessor_description *desc = *p_desc;
    free(desc->shmpath);
    if (desc->pipeline_desc) pipeline_description_destroy(desc->pipeline_desc);
    if (desc->collector_desc) collector_description_destroy(&desc->collector_desc);
    free(desc);
    *p_desc = NULL;
}

