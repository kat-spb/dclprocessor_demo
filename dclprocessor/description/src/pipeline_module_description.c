#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <misc/log.h>
#include <description/pipeline_module_description.h>

static inline int get_type_val(const char *type_str) {
    if (strcmp(type_str, "FIRST") == 0) return MT_FIRST;
    if (strcmp(type_str, "MIDDLE") == 0) return MT_MIDDLE;
    if (strcmp(type_str, "LAST") == 0) return MT_LAST;
    return -1;
}

static inline char *get_type_str(int type) {
    switch (type) {
        case MT_LAST:
            return strdup("LAST");
        case MT_FIRST:
            return strdup("FIRST");
        case MT_MIDDLE:
            return strdup("MIDDLE");
    }
    return NULL;
}

void out_pipeline_module_info(struct pipeline_module_info *minfo) {
    if (!minfo) {
        fprintf(stderr, "[%s]: info is NULL\n", __FUNCTION__);
        fflush(stderr);
        return;
    }
    fprintf(stdout, "[%d] Command string: ", minfo->id);
    for (int i = 0; minfo->argv && minfo->argv[i] != NULL; fprintf(stdout, "argv[%d]=%s ", i, minfo->argv[i]), i++);
    fprintf(stdout, "id: %d, type: %d, next_id: %d, mode: %d\n", minfo->id, minfo->type, minfo->next, minfo->mode);
    fflush(stdout);
}

void clean_pipeline_module_info(struct pipeline_module_info *minfo){
    if (!minfo || !minfo->argv) return;
    for (int i = 0; minfo->argv[i] != NULL; i++) {
        free(minfo->argv[i]);
    }
}

void parse_module_json(struct json_object *module_json, struct pipeline_module_info **p_minfo) {
    if (!p_minfo) return;
    if (!module_json) return;

    //LOG_FD_INFO("--- Begin module json check ---\n");
    //out_module_json(module_json);
    //LOG_FD_INFO("--- End module json check ---\n");

    if (*p_minfo != NULL) {
        LOG_FD_INFO("Module info is not empty. It will be recreated.\n");
        free(*p_minfo);
    }

    struct pipeline_module_info *minfo = (struct pipeline_module_info*)malloc(sizeof(struct pipeline_module_info));
    memset(minfo, 0, sizeof(struct pipeline_module_info));

    struct json_object *current, *param;
    json_object_object_get_ex(module_json, "id", &current);
    if (current) minfo->id = json_object_get_int(current);
    json_object_object_get_ex(module_json, "type", &current);
    if (current) minfo->type = get_type_val(json_object_get_string(current));
    json_object_object_get_ex(module_json, "next", &current);
    if (current) {
        minfo->next = json_object_get_int(current);
    }
    else {
        minfo->next = -1;
    }
    /* part of description, not info
    json_object_object_get_ex(module_json, "config", &current);
    if (current) mdesc->config_fname = strdup(json_object_get_string(current));
    */
    minfo->mode = MT_MODE_INTERNAL;
    json_object_object_get_ex(module_json, "name", &current);
    if (current) {
        //fprintf(stdout, "\tname: %s\n", json_object_get_string(current));
        minfo->mode = MT_MODE_EXTERNAL;
        minfo->argv = (char**)malloc(2 * sizeof(char*));
        minfo->argv[0] = strdup(json_object_get_string(current));
        minfo->argv[1] = NULL;
        json_object_object_get_ex(module_json, "parameters", &current);
        if (json_object_get_type(current) == json_type_array) {
            size_t i, n_params = json_object_array_length(current);
            //fprintf(stdout, "\t\tFound %lu parameters: ", n_params);
            minfo->argv = (char**)realloc(minfo->argv, (n_params + 2) * sizeof(char*));
            for (i = 0; i < n_params; i++) {
                param = json_object_array_get_idx(current, i);
                //if (param) fprintf(stdout, "%s ", json_object_get_string(param));
                minfo->argv[1 + i] = strdup(json_object_get_string(param));
            }
            minfo->argv[n_params + 1] = NULL;
            //fprintf(stdout, "\n");
       }
    }
    //LOG_FD_INFO("--- Begin mdesc check ---\n");
    //out_pipeline_module_info(minfo);
    //LOG_FD_INFO("--- End mdesc check ---\n");
    *p_minfo = minfo;
}

void fill_module_json(struct json_object **p_module_json, struct pipeline_module_info *minfo) {
    if (!minfo) return;
    if (!p_module_json) return;
    struct json_object *jobj = json_object_new_object();
    struct json_object *jarr = json_object_new_array();
    json_object_object_add(jobj, "id", json_object_new_int(minfo->id));
    json_object_object_add(jobj, "type", json_object_new_string(get_type_str(minfo->type)));
    json_object_object_add(jobj, "next", json_object_new_int(minfo->next));
    if (!minfo->argv) return;
    json_object_object_add(jobj, "name", json_object_new_string(minfo->argv[0]));
    for (int i = 1; minfo->argv[i] != NULL; i++) {
        json_object_array_add(jarr, json_object_new_string(minfo->argv[i]));;
    }
    json_object_object_add(jobj, "parameters", jarr);
    /* part of description, not info */
    //if (mdesc->config_fname) json_object_object_add(jobj, "config", json_object_new_string(mdesc->config_fname));
    *p_module_json = jobj;
}

void out_module_json(struct json_object *module_json) {
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

