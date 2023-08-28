#include <stdio.h>
#include <string.h>

#include <misc/log.h>
#include <description/pipeline_description.h>

//move from pipeline_description
void fill_modules_json(struct json_object **p_modules_json, struct pipeline_description *pdesc) {
    if (!pdesc) return; //TODO: error message
    if (!p_modules_json) return;
    struct json_object *jarr = json_object_new_array();
    for (int i = 0; i < pdesc->modules_cnt; i++) {
        struct json_object *jobj = NULL;
        fill_module_json(&jobj, pdesc->minfos[i]);
        if (jobj) json_object_array_add(jarr, jobj);
    }
    *p_modules_json = jarr;
}

void fill_pl_json(struct json_object **p_pl_json, struct pipeline_description *pdesc) {
    if (!pdesc) return; //TODO: error message
    if (!p_pl_json) return; 
    struct json_object *jobj = json_object_new_object();
    //if (pdesc->pl_config) {
    //    *p_pl_json = json_object_from_file(pdesc->pl_config);
    //    return;
    //}
    struct json_object *pl_jobj = json_object_new_object();
    if (pdesc->id_string) json_object_object_add(pl_jobj, "id_string", json_object_new_string(pdesc->id_string));
    if (pdesc->shmpath) {
        json_object_object_add(pl_jobj, "shmem_segment_name", json_object_new_string(pdesc->shmpath));
    }
    else {
        LOG_FD_ERROR("no shmem path defined\n");
        return;
    }
    json_object_object_add(pl_jobj, "data_chunk_size", json_object_new_int(pdesc->data_size));
    json_object_object_add(pl_jobj, "data_chunk_count", json_object_new_int(pdesc->data_cnt));
    struct json_object *m_jarr = NULL;
    fill_modules_json(&m_jarr, pdesc);
    //parse_modules_json(m_jarr);
    json_object_object_add(pl_jobj, "modules", m_jarr);
    json_object_object_add(jobj, "pipeline", pl_jobj);
    *p_pl_json = jobj;
}

//we have got a filled description (from config file, command string or stdin) and want to write it to shmem or other place
size_t  write_pipeline_description_to_buffer(char **p_buffer, struct pipeline_description *pdesc) {
    char *buffer = *p_buffer;
    if (buffer) free(buffer);
    size_t size = 0;
    struct json_object *pl_json = NULL;
    fill_pl_json(&pl_json, pdesc);
    if (!pl_json) return 0;
    buffer = strdup(json_object_to_json_string_ext(pl_json, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
    size = strlen(buffer);
    LOG_FD_INFO("Final size = %zd bytes\n", size);
    *p_buffer = buffer;
    return size;
}

void save_pipeline_configuration(const char *fname, struct pipeline_description *desc) {
    struct json_object *pl_json = json_object_new_object();
    fill_pl_json(&pl_json, desc);
    json_object_to_file_ext(fname, pl_json, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY);
}

size_t pipeline_description_get_size(struct pipeline_description *pdesc) {
    char *buffer = NULL;
    //TODO: if not configured
    return write_pipeline_description_to_buffer(&buffer, pdesc);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s path/src_config.json path/dst_config", argv[0]);
    }
    struct pipeline_description *pdesc = NULL;
    load_pipeline_configuration_from_file(argv[1], &pdesc);
    printf("----- Begin load configuration check -----\n");
    out_pipeline_description(pdesc);
    printf("----- End load configuration check -----\n");
    char *buffer = NULL;
    size_t size = write_pipeline_description_to_buffer(&buffer, pdesc);
    printf("----- Begin buffer (%zd bytes) -----\n", size);
    printf("%s\n", buffer);
    printf("----- End buffer -----\n");
    pipeline_description_destroy(pdesc);
    pdesc = NULL;
    load_pipeline_configuration_from_buffer(buffer, &pdesc);
    printf("----- Begin read configuration check -----\n");
    out_pipeline_description(pdesc);
    printf("----- End read configuration check -----\n");
    save_pipeline_configuration(argv[2], pdesc);
    return 0;
}
