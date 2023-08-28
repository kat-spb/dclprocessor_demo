/*
 * An example of json string parsing with json-c.
 *
 *  gcc -Wall -g -I/usr/include/json-c/ -o system_json system_json.c -ljson-c
 */

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <misc/log.h>
#include <misc/file.h>
#include "description/description_json.h"

void print_object_json(struct json_object *jobj, const char *msg) {
    fprintf(stdout, "%s:\n", msg);
    fprintf(stdout, "%s\n---\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
    fflush(stdout);
}

struct json_object * find_in_json(struct json_object *jobj, const char *key) {
    struct json_object *tmp;
    json_object_object_get_ex(jobj, key, &tmp);
    return tmp;
}

int save_object_json(struct json_object *jobj, char *fname) {
    if (!jobj) {
        fprintf(stderr, "Couldn't save empty object\n");
        return -1;
    }
    int rc = json_object_to_file_ext(fname, jobj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY);
    json_object_put(jobj);
    return rc;
}

struct json_object *load_object_json(char *fname) {
    struct json_object *jobj = json_object_from_file(fname);
    //fprintf(stdout, "%s\n---\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
    return jobj;
}

//{"id":0, "name": null, "parameters": [], "type": "LAST", "next": null, "config": null},
struct json_object *scan_module_json() {
    struct json_object *jobj = json_object_new_object();
    struct json_object *jarr = json_object_new_array();
    char c, *str = NULL;
    int val;
    printf("ID: ");
    scanf("%d", &val);
    json_object_object_add(jobj, "id", json_object_new_int(val));
    printf("Starting string with parameters: ");
    scanf("%ms", &str);
    json_object_object_add(jobj, "name", json_object_new_string(str));
    free(str);
    while (scanf("%ms%c", &str, &c)) {
        json_object_array_add(jarr, json_object_new_string(str));
        free(str);
        if (c == '\n') break;
    }
    fprintf(stdout, "---\n%s\n---\n", json_object_to_json_string(jarr));
    json_object_object_add(jobj, "parameters", jarr);
    printf("Type (one of FIRST/MIDDLE/LAST): ");
    scanf("%ms", &str);
    //val = get_type_val(str);
    //json_object_object_add(jobj, "type", json_object_new_int(val));
    json_object_object_add(jobj, "type", json_object_new_string(str));
    printf("Next module ID: ");
    scanf("%d", &val);
    json_object_object_add(jobj, "next", json_object_new_int(val));
    printf("Module config file name: ");
    scanf("%ms", &str);
    json_object_object_add(jobj, "config", json_object_new_string(str));
    free(str);
    return jobj;
}

int scan_modules_json(struct json_object **p_json) {
    struct json_object *jarr = json_object_new_array();
    char *str = NULL;
    int cnt = 0;
    while (1) {
        printf("Could you want to add a new module (Y/N)? ");
        scanf("%ms", &str);
        if (str[0] == 'Y' || str[0] == 'y') {
            struct json_object *jobj = NULL;
            scan_module_json(&jobj);
            if (jobj) {
                json_object_array_add(jarr, jobj);
                cnt++;
            }
        }
        if (str[0] == 'N' || str[0] == 'n') {
            break;
        }
    }
    *p_json = jarr;
    return cnt;
}

struct json_object *scan_pipeline_json() {
    struct json_object *jobj = json_object_new_object();
    char *str = NULL;
    int val = 0;
    printf("Identification string: ");
    scanf("%ms", &str);
    json_object_object_add(jobj, "id_string", json_object_new_string(str));
    free(str);
    str = NULL;
    printf("Shmem segment name: ");
    scanf("%ms", &str);
    while (!str || strlen(str) == 0 || str[0] != '/') {
        printf("Please, input correct name of shmpath (should start with '/')\n");
        scanf("%ms", &str);
    }
    if (strlen(str) > 1) {
        json_object_object_add(jobj, "shmem_segment_name", json_object_new_string(str));
        printf("Data chunk size: ");
        scanf("%d", &val);
        json_object_object_add(jobj, "data_chunk_size", json_object_new_int(val));
        printf("Data chunk count: ");
        scanf("%d", &val);
        json_object_object_add(jobj, "data_chunk_count", json_object_new_int(val));
    }
    struct json_object *jarr = NULL;
    int n = scan_modules_json(&jarr);
    while (n == 0) {
        fprintf(stdout, "The pipeline must have at least one source\n");
        n = scan_modules_json(&jarr);
    }
    //parse_modules_json(m_jarr);
    json_object_object_add(jobj, "modules", jarr);
    return jobj;
}


struct json_object *scan_frame_json() {
    struct json_object *jobj = json_object_new_object();
    int val;
    printf("width = ");
    scanf("%d", &val);
    json_object_object_add(jobj, "width", json_object_new_int(val));
    printf("height = ");
    scanf("%d", &val);
    json_object_object_add(jobj, "height", json_object_new_int(val));
    printf("format = ");
    scanf("%d", &val);
    json_object_object_add(jobj, "format", json_object_new_int(val));
    return jobj;
}

struct json_object *scan_source_json() {
    struct json_object *jobj = json_object_new_object();
    struct json_object *frame_jobj = NULL;
    char *str = NULL;
    printf("Source type: ");
    scanf("%ms", &str);
    json_object_object_add(jobj, "type", json_object_new_string(str));
    free(str);
    printf("Identification string (without spaces): ");
    scanf("%ms", &str);
    json_object_object_add(jobj, "id_string", json_object_new_string(str));
    free(str);
    printf("Host: ");
    scanf("%ms", &str);
    json_object_object_add(jobj, "host", json_object_new_string(str));
    free(str);
    printf("Port: ");
    scanf("%ms", &str);
    json_object_object_add(jobj, "port", json_object_new_string(str));
    free(str);
    printf("Frame: \n");
    scan_frame_json(&frame_jobj);
    json_object_object_add(jobj, "frame", frame_jobj);
    printf("Source config file name: ");
    int n = scanf("%ms", &str);
    if (str && n > 1) {
        json_object_object_add(jobj, "source_config", json_object_new_string(str));
    }
    free(str);
    return jobj;
}

int scan_sources_json(struct json_object **p_sources_json) {
    struct json_object *jarr = json_object_new_array();
    char *str = NULL;
    int cnt = 0;
    while (1) {
        printf("Could you want to add a new source (Y/N)? ");
        scanf("%ms", &str);
        if (str[0] == 'Y' || str[0] == 'y') {
            struct json_object *jobj = NULL;
            scan_source_json(&jobj);
            if (jobj) {
                json_object_array_add(jarr, jobj);
                cnt++;
            }
        }
        if (str[0] == 'N' || str[0] == 'n') {
            break;
        }
    }
    *p_sources_json = jarr;
    return cnt;
}

struct json_object *scan_collector_json() {
    struct json_object *collector_jobj = json_object_new_object();
    char *str = NULL;
    int val = 0;
    fprintf(stdout, "Boost segment name (required): ");
    int n = scanf("%ms", &str);
    if (str && n > 1) {
        json_object_object_add(collector_jobj, "boost_segment_name", json_object_new_string(str));
    }
    free(str);
    str = NULL;
    struct json_object *jarr = NULL;
    n = scan_sources_json(&jarr);
    while (n == 0) {
        fprintf(stdout, "The collector must have at least one source\n");
        n = scan_sources_json(&jarr);
    }
    //parse_sources_json(jarr);
    json_object_object_add(collector_jobj, "sources", jarr);
    printf("Max frames in object: ");
    scanf("%d", &val);
    json_object_object_add(collector_jobj, "max_frames_count", json_object_new_int(val));
    printf("Object data size (in bytes): ");
    scanf("%d", &val);
    json_object_object_add(collector_jobj, "object_data_size", json_object_new_int(val));
    n = scanf("%ms", &str);
    if (str && n > 1) {
        json_object_object_add(collector_jobj, "output_path", json_object_new_string(str));
    }
    free(str);
    return collector_jobj;
}

struct json_object *scan_messenger_json() {
    struct json_object *jobj = json_object_new_object();
    //TODO: scan proxy/client/server configurations
    fprintf(stdout, "No messenger configuration now\n");
    return jobj;
}

struct json_object *scan_dclprocessor_json() {
    struct json_object *jobj = json_object_new_object();
    char *str = NULL;
    printf("Could you want to add a pipeline config file (Y/N)? ");
    while (1) {
        scanf("%ms", &str);
        if (str[0] == 'Y' || str[0] == 'y') {
            scanf("%ms", &str);
            if (str && strlen(str) > 0) {
                json_object_object_add(jobj, "pipeline_config", json_object_new_string(str));
            }
            break;
        }
        if (str[0] == 'N' || str[0] == 'n') {
            struct json_object *pipeline_jobj = scan_pipeline_json();
            json_object_object_add(jobj, "pipeline", pipeline_jobj);
            break;
        }
        printf("Incorrect input. Please, input Y or N: ");
    }
    free(str);
    str = NULL;
    printf("Could you want to add a collector config file (Y/N)? ");
    while (1) {
        scanf("%ms", &str);
        if (str[0] == 'Y' || str[0] == 'y') {
            scanf("%ms", &str);
            if (str && strlen(str) > 0) {
                json_object_object_add(jobj, "collector_config", json_object_new_string(str));
            }
            break;
        }
        if (str[0] == 'N' || str[0] == 'n') {
            struct json_object *collector_jobj = scan_collector_json();
            json_object_object_add(jobj, "collector", collector_jobj);
            break;
        }
        printf("Incorrect input. Please, input Y or N: ");
    }
    free(str);
    str = NULL;
    printf("Could you want to add a messenger config file (Y/N)? ");
    while (1) {
        scanf("%ms", &str);
        if (str[0] == 'Y' || str[0] == 'y') {
            scanf("%ms", &str);
            if (str && strlen(str) > 0) {
                json_object_object_add(jobj, "messenger_config", json_object_new_string(str));
            }
            break;
        }
        if (str[0] == 'N' || str[0] == 'n') {
            struct json_object *messenger_jobj = scan_messenger_json();
            json_object_object_add(jobj, "messenger", messenger_jobj);
            break;
        }
        printf("Incorrect input. Please, input Y or N: ");
    }
    free(str);
    str = NULL;
    return jobj;
}

struct json_object *scan_system_json() {
    struct json_object *jobj = json_object_new_object();
    char *str = NULL;
    printf("Could you want to add a dclprocessor config file (Y/N)? ");
    while (1) {
        scanf("%ms", &str);
        if (str[0] == 'Y' || str[0] == 'y') {
            scanf("%ms", &str);
            if (str && strlen(str) > 0) {
                json_object_object_add(jobj, "dclprocessor_config", json_object_new_string(str));
            }
            break;
        }
        if (str[0] == 'N' || str[0] == 'n') {
            struct json_object *dclprocessor_jobj = scan_dclprocessor_json();
            json_object_object_add(jobj, "dclprocessor", dclprocessor_jobj);
            break;
        }
        printf("Incorrect input. Please, input Y or N: ");
    }
    free(str);
    return jobj;
}

struct json_object *get_messenger_json(struct json_object *jobj) {
    struct json_object *current_jobj = NULL;
    //messenger subobject
    struct json_object *messenger_json;
    struct json_object *messenger_config_json;
    json_object_object_get_ex(jobj, "messenger_config", &messenger_config_json);
    if (messenger_config_json) {
        fprintf(stdout, "Messenger config: %s\n---\n", json_object_get_string(messenger_config_json));
        current_jobj = json_object_from_file(json_object_get_string(messenger_config_json));
    }
    else {
        fprintf(stdout, "Messenger config is null\n");
        current_jobj = jobj;
    }
    json_object_object_get_ex(current_jobj, "messenger", &messenger_json);
    if (messenger_json == NULL) {
        fprintf(stdout, "No messenger in the current object\n");
    }
    return messenger_json;
}

struct json_object *get_collector_json(struct json_object *jobj) {
    struct json_object *current_jobj = NULL;
    //collector subobject
    struct json_object *collector_json;
    struct json_object *collector_config_json;
    json_object_object_get_ex(jobj, "collector_config", &collector_config_json);
    if (collector_config_json) {
        fprintf(stdout, "Collector config: %s\n---\n", json_object_get_string(collector_config_json));
        current_jobj = json_object_from_file(json_object_get_string(collector_config_json));
    }
    else {
        fprintf(stdout, "Collector config is null\n");
        current_jobj = jobj;
    }
    json_object_object_get_ex(current_jobj, "collector", &collector_json);
    if (collector_json == NULL) {
        fprintf(stdout, "No collector in this system config\n");
    }
    return collector_json;
}

struct json_object *get_pipeline_json(struct json_object *jobj) {
    struct json_object *current_jobj = NULL;
    //pipeline subobject
    struct json_object *pipeline_json;
    struct json_object *pipeline_config_json;
    json_object_object_get_ex(jobj, "pipeline_config", &pipeline_config_json);
    if (pipeline_config_json) {
        fprintf(stdout, "Pipeline config: %s\n---\n", json_object_get_string(pipeline_config_json));
        current_jobj = json_object_from_file(json_object_get_string(pipeline_config_json));
    }
    else {
        fprintf(stdout, "Pipeline config is null\n");
        current_jobj = jobj;
    }
    json_object_object_get_ex(current_jobj, "pipeline", &pipeline_json);
    if (pipeline_json == NULL) {
        fprintf(stdout, "No pipeline in this system config\n");
    }
    return pipeline_json;
}


struct json_object *get_dclprocessor_json(struct json_object *jobj) {
    struct json_object *current_jobj = NULL;
    //dclprocessor subobject
    struct json_object *dclprocessor_json;
    struct json_object *dclprocessor_config_json;
    json_object_object_get_ex(jobj, "dclprocessor_config", &dclprocessor_config_json);
    if (dclprocessor_config_json) {
        fprintf(stdout, "Dclprocessor config: %s\n---\n", json_object_get_string(dclprocessor_config_json));
        current_jobj = json_object_from_file(json_object_get_string(dclprocessor_config_json));
    }
    else {
        fprintf(stdout, "Dclprocessor config is null\n");
        current_jobj = jobj;
    }
    json_object_object_get_ex(current_jobj, "dclprocessor", &dclprocessor_json);
    if (dclprocessor_json == NULL) {
        fprintf(stdout, "No dclprocessor in this system config\n");
    }
    return dclprocessor_json;
}

//TODO: make this work correct with refcount or unlink from base object
struct json_object *construct_full_object(struct json_object *jobj) {
    struct json_object *system_jobj = json_object_new_object();
    struct json_object *dclprocessor_jobj = get_dclprocessor_json(jobj);
    if (find_in_json(dclprocessor_jobj, "pipeline") == NULL) {
        struct json_object *pipeline_jobj = get_pipeline_json(dclprocessor_jobj);
        json_object_object_add(dclprocessor_jobj, "pipeline", pipeline_jobj);
    }
    else {
        fprintf(stdout, "Pipeline already in the dclprocessor\n");
    }
    if (find_in_json(dclprocessor_jobj, "collector") == NULL) {
        struct json_object *collector_jobj = get_collector_json(dclprocessor_jobj);
        json_object_object_add(dclprocessor_jobj, "collector", collector_jobj);
    }
    else {
        fprintf(stdout, "Collector already in the dclprocessor\n");
    }
    if (find_in_json(dclprocessor_jobj, "messenger") == NULL) {
        struct json_object *messenger_jobj = get_messenger_json(dclprocessor_jobj);
        json_object_object_add(dclprocessor_jobj, "messenger", messenger_jobj);
    }
    else {
        fprintf(stdout, "Messenger already in the dclprocessor\n");
    }
    json_object_object_add(system_jobj, "dclprocessor", dclprocessor_jobj);
    fprintf(stdout, "System jobj:\n");
    fprintf(stdout, "%s\n---\n", json_object_to_json_string_ext(system_jobj, JSON_C_TO_STRING_SPACED |  JSON_C_TO_STRING_PRETTY));
    return system_jobj;
}

size_t write_object_to_buffer(struct json_object *jobj, char **p_buffer) {
    char *buffer = *p_buffer;
    if (buffer) free(buffer);
    size_t size = 0;
    if (!jobj) return 0;
    buffer = strdup(json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
    size = strlen(buffer);
    fprintf(stdout, "Final size = %zd bytes\n", size);
    //LOG_FD_INFO("Final size = %zd bytes\n", size);
    *p_buffer = buffer;
    return size; 
}

struct json_object *read_object_from_buffer(char *buffer, size_t *p_size) {
    struct json_object *jobj = json_tokener_parse(buffer);
    *p_size = strlen(buffer);
    return jobj;
}

void test_create(char *fname) {
    struct json_object *jobj = scan_system_json();
    save_object_json(jobj, fname);
    json_object_put(jobj);
}

void test_load(char *fname) {
    struct json_object *jobj = load_object_json(fname);
    if (jobj) {
        fprintf(stdout, "%s\n---\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_SPACED |  JSON_C_TO_STRING_PRETTY));
    }
    else {
        fprintf(stdout, "Loaded object is null\n");
    }
    json_object_put(jobj);
}

void test_load_save(char *in, char *out) {
    struct json_object *jobj = load_object_json(in);
    if (jobj) fprintf(stdout, "%s\n---\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_SPACED |  JSON_C_TO_STRING_PRETTY));
    save_object_json(jobj, out);
    json_object_put(jobj);
}

void test_save_load(char *fname) {
    struct json_object *jobj = scan_system_json();
    save_object_json(jobj, fname);
    jobj = NULL;
    jobj = load_object_json(fname);
    if (jobj) fprintf(stdout, "%s\n---\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_SPACED |  JSON_C_TO_STRING_PRETTY));
    struct json_object *dclprocessor_jobj = get_dclprocessor_json(jobj);
    if (dclprocessor_jobj) fprintf(stdout, "%s\n---\n", json_object_to_json_string_ext(dclprocessor_jobj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
    struct json_object *pipeline_jobj = get_pipeline_json(dclprocessor_jobj);
    if (jobj) fprintf(stdout, "%s\n---\n", json_object_to_json_string_ext(pipeline_jobj, JSON_C_TO_STRING_SPACED |           JSON_C_TO_STRING_PRETTY));
    //out_pipeline_json(pipeline_jobj);
    json_object_put(pipeline_jobj);
    struct json_object *collector_jobj = get_collector_json(dclprocessor_jobj);
    if (jobj) fprintf(stdout, "%s\n---\n", json_object_to_json_string_ext(collector_jobj, JSON_C_TO_STRING_SPACED |           JSON_C_TO_STRING_PRETTY));
    //out_collector_json(collector_jobj);
    json_object_put(collector_jobj);
    //out_dclprocessor_json(dclprocessor_jobj);
    json_object_put(dclprocessor_jobj);
    //out_system_json(jobj);
    json_object_put(jobj);
}

void test_write_read(struct json_object *jobj) {
    char *buffer = NULL;
    size_t nr = 0, nw = write_object_to_buffer(jobj, &buffer);
    struct json_object *result = read_object_from_buffer(buffer, &nr);
    if (jobj) fprintf(stdout, "%s\n---\n", json_object_to_json_string_ext(result, JSON_C_TO_STRING_SPACED |           JSON_C_TO_STRING_PRETTY));
    if (nr != nw) fprintf(stdout, "WTF\n");
    json_object_put(result);
}

void test_something(char *fname) {
    struct json_object *jobj = load_object_json(fname);
    if (jobj) fprintf(stdout, "%s\n---\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_SPACED |  JSON_C_TO_STRING_PRETTY));
    struct json_object *system_jobj = construct_full_object(jobj);
    test_write_read(system_jobj);
    json_object_put(system_jobj);
    json_object_put(jobj);
    return;
}

