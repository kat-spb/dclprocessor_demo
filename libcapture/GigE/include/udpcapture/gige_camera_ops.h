#ifndef _GIGE_CAMERA_OPS_H_
#define _GIGE_CAMERA_OPS_H_

#ifdef __cplusplus
extern "C" {
#endif

struct gige_camera_ops {
    int (*gvcp_watchdog)(int fd, uint32_t packet_id);
    int (*gvcp_start)(int fd, uint32_t packet_id);
    int (*gvcp_stop)(int fd, uint32_t packet_id);
};

extern struct gige_camera_ops gige_camera_ops_no;
extern struct gige_camera_ops gige_camera_ops_arvfake;
extern struct gige_camera_ops gige_camera_ops_flir_color;
extern struct gige_camera_ops gige_camera_ops_flir_color2;
extern struct gige_camera_ops gige_camera_ops_flir_polar;
extern struct gige_camera_ops gige_camera_ops_lucid_polar;
extern struct gige_camera_ops gige_camera_ops_baumer;

#ifdef __cplusplus
}
#endif

#endif //_GIGE_CAMERA_OPS_H_

