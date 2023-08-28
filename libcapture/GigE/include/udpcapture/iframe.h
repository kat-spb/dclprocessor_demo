#ifndef _IFRAME_H_
#define _IFRAME_H_

#ifdef __cplusplus
extern "C" {
#endif

//#define MAX_FRAME_SIZE (2048 * 2448 * 3 * 2)
#define MAX_FRAME_SIZE (120000000)

struct iframe_data {
    uint64_t frame_id;
    size_t buffer_size;
    char buffer[MAX_FRAME_SIZE];
    //char buffer[];
};

#ifdef __cplusplus
}
#endif

#endif //_IFRAME_H_
