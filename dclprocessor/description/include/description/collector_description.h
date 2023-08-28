#ifndef _COLLECTOR_DESCRIPTION_H_
#define _COLLECTOR_DESCRIPTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SHMEM_NAME_LEN 256

#include <json.h>

struct collector_description {
    int sources_cnt;
    struct source_info **sinfos;
    struct source_description **sdescs;
    int use_boost_shmem;
    char boost_path[MAX_SHMEM_NAME_LEN];
    size_t boost_size; //optional, may be calculated by collector and sources description ???
    char *output_path;
};

static inline size_t calculate_boostmem_size(int max_sources, size_t max_frame_size/*w*h*bps*/, int max_frames) {
    //we want 16M additional for any source
    return  0x100000 * max_sources + max_sources * max_frame_size * max_frames;
}

void out_collector_description(struct collector_description *desc);
struct collector_description *collector_description_create();
void collector_description_destroy(struct collector_description **p_desc);

//------------------------------------------

void parse_collector_json(struct json_object *jobj, struct collector_description *desc);
void out_collector_json(struct json_object *jobj);

void load_collector_configuration_from_file(const char *fname, struct collector_description **p_desc);
void load_collector_configuration(const char *fname, struct collector_description **p_desc);
void save_collector_configuration(const char *fname, struct collector_description *desc);

#ifdef __cplusplus
}
#endif

#endif //_COLLECTOR_DESCRIPTION_H_
