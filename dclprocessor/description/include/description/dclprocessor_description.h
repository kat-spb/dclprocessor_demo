#ifndef _DCLPROCESSOR_DESCRIPTION_H_
#define _DCLPROCESSOR_DESCRIPTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <description/pipeline_description.h>
#include <description/collector_description.h>
//#include <description/messenger_description.h>

struct dclprocessor_description {
    //common shmmem for all modules and pipeline
    int use_shmem;
    char *shmpath;
    size_t shmsize;

    struct pipeline_description *pipeline_desc;
    struct collector_description *collector_desc;
    //struct messenger_description *messenger_desc;

    //TODO: think about -- full dclprocessor json for usage with other components
    char *full_json;
    size_t full_json_size;
};

struct dclprocessor_description *dclprocessor_description_create();
void dclprocessor_description_destroy(struct dclprocessor_description **p_desc);

int parse_dclprocessor_json(struct json_object *dclprocessor_jobj, struct dclprocessor_description *desc);

void load_dclprocessor_description_from_buffer(char *buffer, size_t buffer_size, struct dclprocessor_description **p_desc); 
void load_dclprocessor_description(char *fname, struct dclprocessor_description **pdesc);
void save_dclprocessor_description(char *fname, struct dclprocessor_description *pdesc);

void out_dclprocessor_description(struct dclprocessor_description *desc);

#ifdef __cplusplus
}
#endif

#endif //_DCLPROCESSOR_DESCRIPTION_H_

