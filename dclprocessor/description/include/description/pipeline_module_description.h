#ifndef _PIPELINE_MODULE_DESCRIPTION_H_
#define _PIPELINE_MODULE_DESCRIPTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <json.h>

enum MT_MODE {
    MT_MODE_INTERNAL = 0, //important 0-value, set as default with memset() on create
    MT_MODE_EXTERNAL = 1
};

enum MT_TYPE {
    MT_UNDEFINED = -1,
    MT_FIRST = 1,
    MT_MIDDLE = 2,
    MT_LAST = 0
};

struct pipeline_module_info {
    int id;       //idx of process in the process table
    enum MT_TYPE type; //for choose a type of the thread (source/middle/final)
    int mode;     //MD_MODE of struct pipeline_module, set with initial/first
    int next;     //idx of the next in the pipeline model
    char **argv;  //command string for start module
};

void out_pipeline_module_info(struct pipeline_module_info *minfo);
void clean_pipeline_module_info(struct pipeline_module_info *minfo);

void fill_module_json(struct json_object **p_module_json, struct pipeline_module_info *minfo);
void parse_module_json(struct json_object *module_json, struct pipeline_module_info **p_minfo);
void out_module_json(struct json_object *module_json);

#if 0
struct pipeline_module_description {
    struct pipeline_module_info *module_info;
    char *config_fname; //module configuration filename
};
#endif

#ifdef __cplusplus
}
#endif

#endif //_PIPELINE_MODULE_DESCRIPTION_H_
