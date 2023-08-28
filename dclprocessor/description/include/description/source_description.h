#ifndef _SOURCE_DESCRIPTION_H_
#define _SOURCE_DESCRIPTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <description/description_json.h>

enum source_type {
    STYPE_UNKNOWN = -1,
    STYPE_FILE = 0,     //file
    STYPE_UDP = 1,      //udp
    STYPE_TCP = 2       //tcp
};

static inline char *get_source_type_str(enum source_type type) {
    switch (type) {
        case STYPE_FILE:
            return strdup("file");
        case STYPE_TCP:
            return strdup("tcp");
        case STYPE_UDP:
            return strdup("udp");
        default:
            return NULL;
    }
    return NULL;
}

static inline enum source_type get_source_type_val(const char *type_str) {
    if (strcmp(type_str, "tcp") == 0) return STYPE_TCP;
    if (strcmp(type_str, "udp") == 0) return STYPE_UDP;
    if (strcmp(type_str, "file") == 0) return STYPE_FILE;
    return STYPE_UNKNOWN;
}

//TODO: think about frame info in other place
enum frame_format {
    SFMT_CV8U1 = 1,
    SFMT_CV8U3 = 3
};

struct frame_info {
    size_t width;
    size_t height;
    enum frame_format format;
};

struct source_info {
    enum source_type type;
    //identification string
    char sid_string[256];
    //connection information 
    char host[16];
    char port[8];
    struct frame_info fi;
    char *output_path;
};

struct source_info *source_info_create();
void source_info_destroy(struct source_info **p_info);
void clean_source_info(struct source_info *info);
void out_source_info(struct source_info *info);
int parse_frame_json(struct json_object *frame_json, struct frame_info *info);
int parse_source_json(struct json_object *source_json, struct source_info **p_info);

struct packet_info {
    //packet information
    size_t header_len;
    size_t data_len; //optional information, it can be reading from the control stream or from the image header
};

struct source_description {
    enum source_type type;
    //identification string
    char sid_string[256];
    //connection information 
    char addr[16];
    char port[8];
    //packet information
    size_t header_len;
    size_t data_len; //optional information, it can be reading from the control stream or from the image header
    //image information
    //all optional, it can be readed from the header or frame header
    size_t width;
    size_t height;
    size_t channels; //1 for CV_8UC1 and 3 for CV_8UC3 (cv::Mat constructor parameter)
    //debug_data_save
    char output_path[1024];
};

void create_gige_source_description(char *interface, struct source_description **p_sdesc);
void create_tcp_source_description(char *host, char *port, struct source_description **p_sdesc);

void update_source_description(struct source_description *desc, struct source_info *info);

struct source_description *source_description_create();
void source_description_destroy(struct source_description **p_sdesc);
void out_source_description(struct source_description *sdesc);

#ifdef __cplusplus
}
#endif

#endif //_SOURCE_DESCRIPTION_H_
