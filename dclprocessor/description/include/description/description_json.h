#ifndef _DESCRIPTION_JSON_H_
#define _DESCRIPTION_JSON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <json.h>

void print_object_json(struct json_object *jobj, const char *msg);
struct json_object * find_in_json(struct json_object *jobj, const char *key);

int save_object_json(struct json_object *jobj, char *fname);
struct json_object *load_object_json(char *fname);

size_t write_object_to_buffer(struct json_object *jobj, char **p_buffer);
struct json_object *read_object_from_buffer(char *buffer, size_t *p_size);

struct json_object *scan_module_json();
struct json_object *scan_pipeline_json();

struct json_object *scan_frame_json();
struct json_object *scan_source_json();
struct json_object *scan_collector_json();

struct json_object *scan_messenger_json();

struct json_object *scan_dclprocessor_json();
struct json_object *scan_system_json();

struct json_object *get_messenger_json(struct json_object *jobj);
struct json_object *get_collector_json(struct json_object *jobj);
struct json_object *get_pipeline_json(struct json_object *jobj);
struct json_object *get_dclprocessor_json(struct json_object *jobj);

//TODO: make this work correct with refcount or unlink from base object
struct json_object *construct_full_object(struct json_object *jobj);

void test_create(char *fname);
void test_load(char *fname);
void test_load_save(char *in, char *out);
void test_save_load(char *fname);
void test_write_read(struct json_object *jobj);
void test_something(char *fname);

#ifdef __cplusplus
}
#endif

#endif //_DESCRIPTION_JSON_H_

