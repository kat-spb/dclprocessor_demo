#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <misc/log.h>
#include <description/description_json.h>
#include <description/source_description.h>

struct source_description *source_description_create() {
    struct source_description *desc = (struct source_description *)malloc(sizeof(struct source_description));
    memset(desc, 0, sizeof(struct source_description));
    //LOG_FD_INFO("Created source description = %p\n", desc);
    return desc;
}

void create_tcp_source_description(char *host, char *port, struct source_description **p_sdesc) {
    struct source_description *sdesc = *p_sdesc;
    //LOG_FD_INFO("WTF, sdesc = %p\n", sdesc);
    if (!sdesc) {
        //LOG_FD_INFO("1.0, sdesc = %p, size = %zd\n", sdesc, sizeof(struct source_description));
        sdesc = (struct source_description *)malloc(sizeof(struct source_description));
        //LOG_FD_INFO("1.1, sdesc = %p\n", sdesc);
    }
    //LOG_FD_INFO("1.2, sdesc = %p\n", sdesc);
    sdesc->type = STYPE_TCP;
    sdesc->sid_string[0] = '\0';
    strcpy(sdesc->addr, host);
    strcpy(sdesc->port, port);
    sdesc->header_len = sizeof(size_t) + sizeof(int);
    sdesc->width = 960;
    sdesc->height = 1920;
    sdesc->channels = 3; //CV_8UC3
    sdesc->data_len = sdesc->width * sdesc->height * sdesc->channels;
    strcpy(sdesc->output_path, "/tmp/tmk/data");
    //strcpy(sdesc->output_path, ""); //for disable debug save
    *p_sdesc = sdesc;
    LOG_FD_INFO("Ok, *p_sdesc = %p\n", *p_sdesc);
}

void create_gige_source_description(char *interface, struct source_description **p_sdesc) {
    struct source_description *sdesc = *p_sdesc;
    if (!sdesc) sdesc = (struct source_description *)malloc(sizeof(struct source_description));
    sdesc->type = STYPE_UDP;
    strcpy(sdesc->sid_string, interface);
    sdesc->width = 2448;
    sdesc->height = 2048;
    //sdesc->width = 1920;
    //sdesc->height = 1200;
    sdesc->channels = 1; //CV_8UC1
    sdesc->data_len = sdesc->width * sdesc->height * sdesc->channels;
    strcpy(sdesc->output_path, "/tmp/tmk/data");
    //strcpy(sdesc->output_path, ""); //for disable debug save
    *p_sdesc = sdesc;
}

//TODO: temporary solution for sync source_description with source_info
void update_source_description(struct source_description *desc, struct source_info *info) {
    if (!desc || !info) return;
    strcpy(desc->sid_string, info->sid_string);
    strcpy(desc->addr, info->host);
    strcpy(desc->port, info->port);
    desc->type = info->type;
    desc->width = info->fi.width;
    desc->height = info->fi.height;
    desc->channels = (int)info->fi.format;
    desc->header_len = sizeof(size_t) + sizeof(int);
    desc->data_len = desc->width * desc->height * desc->channels;
    strcpy(desc->output_path, info->output_path);
}

void out_source_info(struct source_info *info) {
    if (!info) return;
    if (info->sid_string[0] == '\0') {
        fprintf(stdout, "\tSource info for host=%s port=%s [%d: %s]:\n", info->host, info->port, info->type, get_source_type_str(info->type));
    }
    else {
        fprintf(stdout, "\tSource info for %s [%d: %s]:\n", info->sid_string, info->type, get_source_type_str(info->type));
    }
    //TODO: format as string
    fprintf(stdout, "\t\tframe info: width = %zd height = %zd format = %u\n", info->fi.width, info->fi.height, info->fi.format);
    fprintf(stdout, "\t\toutput_path = %s\n", info->output_path);
    fflush(stdout);
}

struct source_info *source_info_create() {
    struct source_info *info = (struct source_info *)malloc(sizeof(struct source_info));
    memset(info, 0, sizeof(struct source_info));
    //LOG_FD_INFO("Created source info = %p\n", info);
    return info;
}

void clean_source_info(struct source_info *info){
    if (!info || !info->output_path) return;
    free(info->output_path);
}

void source_info_destroy(struct source_info **p_info) {
    if (!p_info || *p_info == NULL) return;
    struct source_info *info = *p_info;
    if (info->output_path) free(info->output_path);
    //LOG_FD_INFO("Destroyed source info = %p\n", info);
    free(*p_info);
    *p_info = NULL;
}

int parse_frame_json(struct json_object *frame_json, struct frame_info *info) {
    //ATTENTION: info is not pointer in the source_info
    struct json_object *current;
    if (!frame_json) return -1;
    json_object_object_get_ex(frame_json, "width", &current);
    if (current) info->width = json_object_get_int(current);
    json_object_object_get_ex(frame_json, "height", &current);
    if (current) info->height = json_object_get_int(current);
    json_object_object_get_ex(frame_json, "format", &current);
    if (current) info->format = json_object_get_int(current);
    return 0;
}

int parse_source_json(struct json_object *source_json, struct source_info **p_info) {
    if (!source_json) return -1;
    if (!p_info) return -1;
    struct source_info *info = source_info_create();
    struct json_object *current, *frame_json;
    json_object_object_get_ex(source_json, "type", &current);
    if (current) info->type = get_source_type_val(json_object_get_string(current));
    json_object_object_get_ex(source_json, "id_string", &current);
    if (current) strcpy(info->sid_string, json_object_get_string(current));
    json_object_object_get_ex(source_json, "host", &current);
    if (current) strcpy(info->host, json_object_get_string(current));
    json_object_object_get_ex(source_json, "port", &current);
    if (current) strcpy(info->port, json_object_get_string(current));
    json_object_object_get_ex(source_json, "frame", &frame_json);
    if (frame_json) parse_frame_json(frame_json, &(info->fi));
    //json_object_object_get_ex(source_json, "source_config", &current);
    //if (current) strcpy(info->source_config, json_object_get_string(current));
    json_object_object_get_ex(source_json, "output_path", &current);
    if (current) info->output_path = strdup(json_object_get_string(current));
    *p_info = info;
    return 0;
}

void out_source_description(struct source_description *sdesc) {
    if (!sdesc) return;
    if (sdesc->sid_string[0] == '\0') {
        fprintf(stdout, "\tSource description for ip=%s port=%s [type=%d]:\n", sdesc->addr, sdesc->port, sdesc->type);
        fprintf(stdout, "\t\theader_len = %zd\n", sdesc->header_len);
    }
    else {
        fprintf(stdout, "\tSource description for %s [type=%d]:\n", sdesc->sid_string, sdesc->type);
    }
    fprintf(stdout, "\t\twidth = %zd height = %zd channels = %ld\n", sdesc->width, sdesc->height, sdesc->channels);
    fprintf(stdout, "\t\toutput_path = %s\n", sdesc->output_path);
    fflush(stdout);
}

void source_description_destroy(struct source_description **p_sdesc){
    if (!p_sdesc || *p_sdesc == NULL) return;
    free(*p_sdesc); 
    *p_sdesc = NULL;
}


