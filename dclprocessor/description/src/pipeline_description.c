#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <misc/log.h>
#include <misc/file.h>
#include <description/pipeline_module_description.h>
#include <description/pipeline_description.h>
#include <description/description_json.h>

#if 0
//we want at least 64K for any module as default
#define DEFAULT_MODULE_SIZE 0x10000

static size_t calculate_pipeline_size(int max_modules, size_t max_module_data) {
    if (max_module_data < DEFAULT_MODULE_SIZE) {
        return max_modules * DEFAULT_MODULE_SIZE;
    }
    return  max_module_data * max_modules + sizeof(int) * 2 + sizeof(size_t);
}
#endif

int pipeline_description_init(int wait_on_start, char *shmpath, int data_cnt, int data_size, int modules_cnt, struct pipeline_description **p_pdesc) {
    struct pipeline_description *desc = *p_pdesc;
    if (!desc) desc = (struct pipeline_description *)malloc(sizeof(struct pipeline_description));
    memset(desc, 0, sizeof(struct pipeline_description));

    desc->wait_on_start = wait_on_start;

    desc->shmpath = strdup(shmpath);
    desc->data_cnt = data_cnt;
    desc->data_size = data_size;

    desc->modules_cnt = modules_cnt; //number of modules-processer (initial and finalization thread are included)
    desc->proc_cnt = desc->modules_cnt + 1; //number element in process table: (initial and finalization thread are included) + pipeline_process

    desc->minfos = (struct pipeline_module_info **)malloc(desc->modules_cnt * sizeof(struct pipeline_module_info *));
    memset(desc->minfos, 0, desc->modules_cnt * sizeof(struct pipeline_module_info *));

    *p_pdesc = desc;
    return 0;
}

//TODO: think about O_STRUCT | O_JSON
//TODO: make string buffer for output
void out_pipeline_description(struct pipeline_description *desc) {
    if (!desc) {
        fprintf(stderr, "[%s]: desription is NULL\n", __FUNCTION__);
        fflush(stderr);
        return;
    }
    fprintf(stdout, "Pipeline description:\n");
    fprintf(stdout, "\tshmem region path = \"%s\"\n", desc->shmpath);
    //TODO: check that path is started with '\'
    fprintf(stdout, "\tshmem region size = %zd\n", desc->shmsize);
    fprintf(stdout, "\tdata ring buffer count = %d\n", desc->data_cnt);
    //TODO: check that path is started with '\'
    fprintf(stdout, "\tdata size = %zd\n", desc->data_size);
    fprintf(stdout, "\tnumber of modules = %d\n", desc->modules_cnt);
    for (int i = 0; i < desc->modules_cnt; i++) {
        out_pipeline_module_info(desc->minfos[i]);
    }
    fflush(stdout);
#if 0
    fprintf(stdout, "[%s]: Check pipeline configuration json (desc->pl_json = %p):\n", __FUNCTION__, desc->pl_json);
    fflush(stdout);
    out_pl_json(desc->pl_json);
#endif
}

int parse_modules_json(struct json_object *modules_json, struct pipeline_description *desc) {
    struct json_object *module_json;
    if (!modules_json) return 0;
    if (json_object_get_type(modules_json) == json_type_array) {
        size_t i, n_modules = json_object_array_length(modules_json);
        desc->modules_cnt = n_modules;
        desc->minfos = (struct pipeline_module_info **)malloc(desc->modules_cnt * sizeof(struct pipeline_module_info *));
        memset(desc->minfos, 0, desc->modules_cnt * sizeof(struct pipeline_module_info *));
        for (i = 0; i < n_modules; i++) {
            module_json = json_object_array_get_idx(modules_json, i);
            parse_module_json(module_json, &(desc->minfos[i]));
        }
        return desc->modules_cnt;
    }
    return 0;
}

int parse_pipeline_json(struct json_object *jobj, struct pipeline_description *desc) {
    if (!desc) {
        LOG_FD_ERROR("desc is NULL\n");
        return -1;
    }
/*
    const char *key_c = "pipeline_config";
    const char *key = "pipeline";
    struct json_object *pipeline_config_json;
    struct json_object *pipeline_json;
    if (find_in_json(jobj, key_c) != NULL) {
        json_object_object_get_ex(jobj, key_c, &pipeline_config_json);
        LOG_FD_INFO("Found %s config_file: %s\n", key, json_object_get_string(pipeline_config_json));
    }
    if (find_in_json(jobj, key) != NULL) {
        json_object_object_get_ex(jobj, key, &pipeline_json);
        if (pipeline_json == NULL && pipeline_config_json != NULL) {
            pipeline_json = json_object_from_file(json_object_get_string(pipeline_config_json));
        }
    }

    if (!pipeline_json) {
        LOG_FD_ERROR("Object doesn't contain pipeline description\n");
        return -1;
    }
*/
    struct json_object *current;
    struct json_object *modules_json;
    json_object_object_get_ex(jobj, "id_string", &current);
    if (current) desc->id_string = strdup(json_object_get_string(current));
    json_object_object_get_ex(jobj, "shmem_segment_name", &current);
    if (current) desc->shmpath = strdup(json_object_get_string(current));
    json_object_object_get_ex(jobj, "data_chunk_size", &current);
    if (current) desc->data_size = json_object_get_int(current);
    json_object_object_get_ex(jobj, "data_chunk_count", &current);
    if (current) desc->data_cnt = json_object_get_int(current);
    json_object_object_get_ex(jobj, "modules", &modules_json);
    if (modules_json) parse_modules_json(modules_json, desc);
    //LOG_FD_INFO("Ok, %p\n", desc);

    //json_object_put(pipeline_json);
    //json_object_put(pipeline_config_json);
    return 0;
}

void out_modules_json(struct json_object *modules_json) {
    struct json_object *module_json;
    if (!modules_json) return;
    if (json_object_get_type(modules_json) == json_type_array) {
        size_t i, n_modules = json_object_array_length(modules_json);
        fprintf(stdout, "Found %lu modules\n", n_modules);
        fflush(stdout);
        for (i = 0; i < n_modules; i++) {
            module_json = json_object_array_get_idx(modules_json, i);
            fprintf(stdout, "[%lu]: ", i);
            out_module_json(module_json);
            fflush(stdout);
        }
    }
}

void out_pl_json(struct json_object *pl_json) {
    if (!pl_json) LOG_FD_INFO("WTF\n");
    struct json_object *current;
    struct json_object *modules_json;
    json_object_object_get_ex(pl_json, "id_string", &current);
    if (current) fprintf(stdout, "id_string: %s\n", json_object_get_string(current));
    json_object_object_get_ex(pl_json, "shmem_segment_name", &current);
    if (current) fprintf(stdout, "shmem_segment_name: %s\n", json_object_get_string(current));
    json_object_object_get_ex(pl_json, "data_chunk_size", &current);
    if (current) fprintf(stdout, "data_chunk_size: %d\n", json_object_get_int(current));
    json_object_object_get_ex(pl_json, "data_chunk_count", &current);
    if (current) fprintf(stdout, "data_chunk_count: %d\n", json_object_get_int(current));
    json_object_object_get_ex(pl_json, "modules", &modules_json);
    if (modules_json) out_modules_json(modules_json);
    fflush(stdout);
}

void out_pl_json_str(struct json_object *pl_json) {
    LOG_FD_INFO("pl_json = %p\n, %s\n---\n", pl_json, json_object_to_json_string_ext(pl_json, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
}

size_t load_pipeline_configuration_from_buffer(char *buffer, struct pipeline_description **p_desc) {
    if (!p_desc) return -1;
    if (*p_desc) free(*p_desc);
    struct pipeline_description *desc = pipeline_description_create();
    struct json_object *jobj = json_tokener_parse(buffer);
    parse_pipeline_json(jobj, desc);
    *p_desc = desc;
    return strlen(buffer);
}

void load_pipeline_configuration_from_object(struct json_object *jobj, struct pipeline_description **p_desc) {
    if (!p_desc) return;
    if (*p_desc) free(*p_desc);
    struct pipeline_description *desc = pipeline_description_create();
    if (jobj != NULL) {
        parse_pipeline_json(jobj, desc);
        LOG_FD_INFO("Created desc = %p for json = %p\n", desc, jobj);
    }
    else {
        LOG_FD_INFO("Pipeline with predefined configuration is not supported in this version\n");
        free(desc);
        desc = NULL;
    }
    *p_desc = desc;
}

void load_pipeline_configuration_from_file(const char *fname, struct pipeline_description **p_desc) {
    if (!p_desc) return;
    if (*p_desc) free(*p_desc);
    struct pipeline_description *desc = pipeline_description_create();
    LOG_FD_INFO("Try to load \'%s\'\n", fname);
    if (is_exist(fname)) {
        struct json_object *jobj = json_object_from_file(fname);
        parse_pipeline_json(jobj, desc);
        LOG_FD_INFO("Created desc = %p for %s\n", desc, fname);
    }
    else {
        LOG_FD_INFO("Pipeline with predefined configuration is not supported in this version\n");
        free(desc);
        desc = NULL;
    }
    *p_desc = desc;
}

struct pipeline_description *pipeline_description_create() {
    struct pipeline_description *desc = (struct pipeline_description *)malloc(sizeof(struct pipeline_description));
    memset(desc, 0, sizeof(struct pipeline_description));
    LOG_FD_INFO("Created pipeline descriptor = %p\n", desc);
    return desc;
}

void pipeline_description_destroy(struct pipeline_description *pdesc) {
    if (!pdesc) return;
    for (int i = 0; i < pdesc->modules_cnt; i++) {
        struct pipeline_module_info *minfo = pdesc->minfos[i];
        clean_pipeline_module_info(minfo);
        pdesc->minfos[i] = NULL;
    }
    free(pdesc->minfos);
    free(pdesc);
}

