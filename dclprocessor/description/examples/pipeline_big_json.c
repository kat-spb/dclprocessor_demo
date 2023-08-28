/*
 * An example of json string parsing with json-c.
 *
 *  gcc -Wall -g -I/usr/include/json-c/ -o pl_json pipeline_big_json.c -ljson-c
 */

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <json.h>

#define MAX_STRING_VALUE_LENGTH 256

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)

struct json_object *get_parsed_json(char *fname) {
    char *addr;
    int fd;
    struct stat sb;
    size_t length;
    struct json_object *jobj;

    fd = open(fname, O_RDONLY);
    if (fd == -1) {
        handle_error("open for read");
    }

    //Obtain file size
    if (fstat(fd, &sb) == -1) {
        handle_error("fstat");
    }
    length = sb.st_size;

    addr = mmap(NULL, length, PROT_READ,
                       MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        handle_error("mmap");
    }
    close(fd);

    jobj = json_tokener_parse(addr);

    munmap(addr, length); 
    return jobj;
}

void parse_module_json(struct json_object *module_json) {
    struct json_object *current, *param;
    if (!module_json) return;
    json_object_object_get_ex(module_json, "id", &current);
    if (current) fprintf(stdout, "\tid: %d\n", json_object_get_int(current));
    json_object_object_get_ex(module_json, "name", &current);
    if (current) fprintf(stdout, "\tname: %s\n", json_object_get_string(current));
    json_object_object_get_ex(module_json, "parameters", &current);
    if (json_object_get_type(current) == json_type_array) {
        size_t i, n_params = json_object_array_length(current);
        fprintf(stdout, "\t\tFound %lu parameters: ", n_params);
        for (i = 0; i < n_params; i++) {
            param = json_object_array_get_idx(current, i);
            if (param) fprintf(stdout, "%s ", json_object_get_string(param));
        }
        fprintf(stdout, "\n");
    }
    json_object_object_get_ex(module_json, "type", &current);
    if (current) fprintf(stdout, "\ttype: %s\n", json_object_get_string(current));
    json_object_object_get_ex(module_json, "next", &current);
    if (current) {
        fprintf(stdout, "\tnext: %d\n", json_object_get_int(current));
    }
    else {
        fprintf(stdout, "\tnext: %d\n", -1);
    }
}

void parse_modules_json(struct json_object *modules_json) {
    struct json_object *module_json;
    if (!modules_json) return;
    if (json_object_get_type(modules_json) == json_type_array) {
        size_t i, n_modules = json_object_array_length(modules_json);
        fprintf(stdout, "Found %lu modules\n", n_modules);
        for (i = 0; i < n_modules; i++) {
            module_json = json_object_array_get_idx(modules_json, i);
            fprintf(stdout, "[%lu]: ", i);
            parse_module_json(module_json);
        }
    }
}

void parse_pl_json(struct json_object *pl_json) {
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
    if (modules_json) parse_modules_json(modules_json);
}

int get_type_val(char *type_str) {
    if (strcmp(type_str, "FIRST") == 0) return 1;
    if (strcmp(type_str, "MIDDLE") == 0) return 2;
    if (strcmp(type_str, "LAST") == 0) return 0;
    return -1;
}

char *get_type_str(int type) {
    switch (type) {
        case 0:
            return strdup("LAST");
        case 1:
            return strdup("FIRST");
        case 2:
            return strdup("MIDDLE");
    }
    return NULL;
}

//{"id":0, "name": null, "parameters": [], "type": "LAST", "next": null, "config": null},
void scan_module_json(struct json_object **p_module_json) {
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
    printf("---\n%s\n---\n", json_object_to_json_string(jarr));
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
    *p_module_json = jobj;
}

void scan_modules_json(struct json_object **p_modules_json) {
    struct json_object *jarr = json_object_new_array();
    char *str;
    while (1) {
        printf("Could you want to add a new module (Y/N)? ");
        scanf("%ms", &str);
        if (str[0] == 'Y' || str[0] == 'y') {
            struct json_object *jobj = NULL;
            scan_module_json(&jobj);
            if (jobj) json_object_array_add(jarr, jobj);
        }
        if (str[0] == 'N' || str[0] == 'n') {
            break;
        }
    }
    *p_modules_json = jarr;
}

void scan_pl_json(struct json_object **p_pl_json) {
    struct json_object *jobj = json_object_new_object();
    char *str = NULL;
    int val;
    int has_config = 0;
    printf("Could you want to add a pipeline config file (Y/N)? ");
    while (1) {
        scanf("%ms", &str);
        if (str[0] == 'Y' || str[0] == 'y') {
            scanf("%ms", &str);
            if (str && strlen(str) > 0) {
                json_object_object_add(jobj, "pipeline_config", json_object_new_string(str));
                has_config = 1;
            }
            break;
        }
        if (str[0] == 'N' || str[0] == 'n') {
            struct json_object *pl_jobj = json_object_new_object();
            printf("Identification string: ");
            scanf("%ms", &str);
            json_object_object_add(pl_jobj, "id_string", json_object_new_string(str));
            printf("Shmem segment name: ");
            scanf("%ms", &str);
            while (!has_config && (!str || strlen(str) == 0 || str[0] != '/')) {
                printf("Please, input correct name of shmpath (should start with '/')\n");
                scanf("%ms", &str);
            }
            if (strlen(str) > 1) {
                json_object_object_add(pl_jobj, "shmem_segment_name", json_object_new_string(str));
                printf("Data chunk size: ");
                scanf("%d", &val);
                json_object_object_add(pl_jobj, "data_chunk_size", json_object_new_int(val));
                printf("Data chunk count: ");
                scanf("%d", &val);
                json_object_object_add(pl_jobj, "data_chunk_count", json_object_new_int(val));
            }
            struct json_object *m_jarr = NULL;
            scan_modules_json(&m_jarr);
            //parse_modules_json(m_jarr);
            json_object_object_add(pl_jobj, "modules", m_jarr);
            json_object_object_add(jobj, "pipeline", pl_jobj);
            break;
        }
        printf("Incorrect input. Please, input Y or N: ");
    }
    free(str);
    *p_pl_json = jobj;
}

void test_read_json(char *fname) {
    struct json_object *jobj = json_object_from_file(fname);

    //fprintf(stdout, "%s\n---\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));

    struct json_object *pl_config_json;
    struct json_object *pl_json;
    json_object_object_get_ex(jobj, "pipeline_config", &pl_config_json);
    fprintf(stdout, "Pipeline config: %s\n---\n", json_object_get_string(pl_config_json));
    json_object_object_get_ex(jobj, "pipeline", &pl_json);
    if (pl_json == NULL && pl_config_json != NULL) {
        pl_json = json_object_from_file(json_object_get_string(pl_config_json));
    }
    parse_pl_json(pl_json);
    //TODO: check if both -- config and json
    if (pl_json) fprintf(stdout, "%s\n---\n", json_object_to_json_string_ext(pl_json, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
    json_object_put(pl_json);
    json_object_put(pl_config_json);
    json_object_put(jobj);
}

void test_write_json(char *fname) {
    struct json_object *pl_json = NULL;
    scan_pl_json(&pl_json);
    if (!pl_json) return;
    json_object_to_file_ext(fname, pl_json, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY);
    json_object_put(pl_json);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "%s file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    test_write_json(argv[1]);
    test_read_json(argv[1]);

    return 0;
}

