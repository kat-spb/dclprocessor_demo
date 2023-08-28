#ifndef _PIPELINE_DESCRIPTION_H_
#define _PIPELINE_DESCRIPTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <description/pipeline_module_description.h>

//TODO: temporary solution -- think about desc_mngr
#define DESCRIPTOR_MAX_CNT          4
#define DESCRIPTOR_DATA_MAX_LEN     16384 //16K

struct pipeline_description {
    int use_common_shmem; //use or not common shmem
    char *shmpath;
    size_t shmsize;

    char *id_string;

    int modules_cnt;
    struct pipeline_module_info **minfos;

    //TODO: think about two -- modules_cnt and proc_cnt (not evidence)
    int proc_cnt;

    int data_cnt;
    size_t data_size;

    int wait_on_start; //in seconds, 0 is possible

    //TODO: think about this implementation -- may be desc_mngr
    int put_to_shmem;
    int desc_zones_cnt;
    size_t desc_zone_size;

};

//May be not actual in current version
int pipeline_description_init(int wait_on_start, char *shmpath, int data_cnt, int data_size, int modules_cnt, struct pipeline_description **p_pdesc);

struct pipeline_description *pipeline_description_create();
void pipeline_description_destroy(struct pipeline_description *pdesc);
void out_pipeline_description(struct pipeline_description *desc);

//main parser for description
int parse_pipeline_json(struct json_object *jobj, struct pipeline_description *pdesc);

//from buffer
size_t load_pipeline_configuration_from_buffer(char *buffer, struct pipeline_description **p_desc);
//from jobj
void load_pipeline_configuration_from_object(struct json_object *jobj, struct pipeline_description **p_desc);
//from json file
void load_pipeline_configuration_from_file(const char *fname, struct pipeline_description **p_pdesc);

#ifdef __cplusplus
}
#endif

#endif //_PIPELINE_DESCRIPTION_H_

