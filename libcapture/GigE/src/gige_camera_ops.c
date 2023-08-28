#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "udpcapture/gvcp.h"
#include "udpcapture/gige_camera_ops.h"

//---------------------------------------------------
// temporary solution
//---------------------------------------------------

//no action for camera -- ok??
int gvcp_watchdog_no(int fd, uint32_t packet_id) {
    (void)fd;
    (void)packet_id;
    fprintf(stdout, "[gvcp_watchdog_no]: watchdog command\n");
    fflush(stdout);
    return 0;
}

int gvcp_start_no(int fd, uint32_t packet_id) {
    (void)fd;
    (void)packet_id;
    fprintf(stdout, "[gvcp_start_no]: start command\n");
    fflush(stdout);
    return 0;
}

int gvcp_stop_no(int fd, uint32_t packet_id) {
    (void)fd;
    (void)packet_id;
    fprintf(stdout, "[gvcp_stop_no]: stop command\n");
    fflush(stdout);
    return 0;
}

//aravis fake -- ok
int gvcp_watchdog_arvfake(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    read_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value, next_packet_id(packet_id));
    if (value != 0x02) write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    //read_register(fd, GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
    return 0;
}

int gvcp_start_arvfake(int fd, uint32_t packet_id) {
    //uint32_t value = -1;
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    write_register(fd, 0x00000124, 0x01, next_packet_id(packet_id)); 
    //read_register(fd, GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
    return 0;
}

int gvcp_stop_arvfake(int fd, uint32_t packet_id) {
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    write_register(fd, 0x00000124, 0x00, next_packet_id(packet_id)); 
    return 0;
}

//flir-color  -- ok
int gvcp_watchdog_flir_color(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    read_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value, next_packet_id(packet_id));
    if (value != 0x02) write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
    write_register(fd, 0xF0F04030, 0x80000000, next_packet_id(packet_id)); 
    read_register(fd, GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
    return 0;
}

int gvcp_start_flir_color(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    write_register(fd, 0xF0F00A08, 0x00000000, next_packet_id(packet_id));
    read_register(fd,  0xF0F00A0C, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F00A0C, value, next_packet_id(packet_id));
    write_register(fd, 0xF0F00A08, 0x00000000, next_packet_id(packet_id));
    read_register(fd,  0xF0F00960, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F00964, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F00530, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F00830, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F00830, 0x80000000, next_packet_id(packet_id));
    read_register(fd,  0xF0F0083C, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F0083C, value, next_packet_id(packet_id));
    read_register(fd,  0xF0F0053C, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F0083C, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F0083C, 0xC2000610, next_packet_id(packet_id));
    write_register(fd, 0xF0F00968, 0x41200000, next_packet_id(packet_id)); //?Physical link configuration
    read_register(fd,  0xF0F05410, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F04050, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F04054, &value, next_packet_id(packet_id));

    read_register(fd, GV_STREAM_CHANNEL_0_PORT_OFFSET, &value, next_packet_id(packet_id)); //+
    read_register(fd,  GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F04030, 0x80000000, next_packet_id(packet_id));
    return 0;
}

int gvcp_stop_flir_color(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    read_register(fd,  0xF0F00614, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F00614, 0x00000000, next_packet_id(packet_id));
    return 0;
}

//flir-color2
int gvcp_watchdog_flir_color2(int fd, uint32_t packet_id) {
#if 0
    uint32_t value = -1;
    read_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value, next_packet_id(packet_id));
    if (value != 0x02) write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
    //write_register(fd, 0x000C0004, 0x00000001, next_packet_id(packet_id));
    read_register(fd, GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
#else
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
#endif
    return 0;
}


int gvcp_start_flir_color2(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    write_register(fd, 0x00081044, 0x00000000, next_packet_id(packet_id));
    write_register(fd, 0x00081024, 0x00000000, next_packet_id(packet_id));
    //write_register(fd, 0x00081084, 0x000000C8, next_packet_id(packet_id)); //area region
    //write_register(fd, 0x00081064, 0x000000C8, next_packet_id(packet_id)); //area region
    read_register(fd,  0x00081084, &value, next_packet_id(packet_id));
    read_register(fd,  0x00081084, &value, next_packet_id(packet_id));
    write_register(fd, 0x00081044, 0x00000000, next_packet_id(packet_id));
    write_register(fd, 0x00081024, 0x00000000, next_packet_id(packet_id));
    read_register(fd,  0x000C1108, &value, next_packet_id(packet_id));
    read_register(fd,  0x000C1148, &value, next_packet_id(packet_id));
    read_register(fd,  0x000C8C40, &value, next_packet_id(packet_id));
    read_register(fd,  0x000C8C20, &value, next_packet_id(packet_id));
    read_register(fd,  0x000C8C00, &value, next_packet_id(packet_id));
    write_register(fd, 0x000C8424, 0x00000000, next_packet_id(packet_id));
    write_register(fd, 0x000C1124, 0x00000001, next_packet_id(packet_id));
    read_register(fd,  0x20002008, &value, next_packet_id(packet_id));

    read_register(fd, GV_STREAM_CHANNEL_0_PORT_OFFSET, &value, next_packet_id(packet_id)); //+
    read_register(fd,  GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
    write_register(fd, 0x000C0004, 0x00000001, next_packet_id(packet_id));
    return 0;
}

int gvcp_stop_flir_color2(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    read_register(fd,  0x000C0004, &value, next_packet_id(packet_id));
    write_register(fd, 0x000C0024, 0x00000001, next_packet_id(packet_id));
    return 0;
}

//Baumer
int gvcp_watchdog_baumer(int fd, uint32_t packet_id) {
#if 0
    uint32_t value = -1;
    read_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value, next_packet_id(packet_id));
    if (value != 0x801) write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x801, next_packet_id(packet_id));
    //write_register(fd, 0x000C0004, 0x00000001, next_packet_id(packet_id));
    read_register(fd, GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
#else
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x801, next_packet_id(packet_id));
#endif
    return 0;
}


int gvcp_start_baumer(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x803, next_packet_id(packet_id)); 
    //write_register(fd, 0x00081044, 0x00000000, next_packet_id(packet_id));
    //write_register(fd, 0x00081024, 0x00000000, next_packet_id(packet_id));
    //write_register(fd, 0x00081084, 0x000000C8, next_packet_id(packet_id)); //area region
    //write_register(fd, 0x00081064, 0x000000C8, next_packet_id(packet_id)); //area region
    //read_register(fd,  0x00081084, &value, next_packet_id(packet_id));
    //read_register(fd,  0x00081084, &value, next_packet_id(packet_id));
    //write_register(fd, 0x00081044, 0x00000000, next_packet_id(packet_id));
    //write_register(fd, 0x00081024, 0x00000000, next_packet_id(packet_id));
    //read_register(fd,  0x000C1108, &value, next_packet_id(packet_id));
    //read_register(fd,  0x000C1148, &value, next_packet_id(packet_id));
    //read_register(fd,  0x000C8C40, &value, next_packet_id(packet_id));
    //read_register(fd,  0x000C8C20, &value, next_packet_id(packet_id));
    //read_register(fd,  0x000C8C00, &value, next_packet_id(packet_id));
    //write_register(fd, 0x000C8424, 0x00000000, next_packet_id(packet_id));
    //write_register(fd, 0x000C1124, 0x00000001, next_packet_id(packet_id));
    //read_register(fd,  0x20002008, &value, next_packet_id(packet_id));

    read_register(fd, GV_STREAM_CHANNEL_0_PORT_OFFSET, &value, next_packet_id(packet_id)); //+
    read_register(fd,  GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
    write_register(fd, 0x82000000, 0x00000801, next_packet_id(packet_id));
    read_register(fd, 0x820001004, &value, next_packet_id(packet_id));
    return 0;
}

int gvcp_stop_baumer(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    read_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x801, next_packet_id(packet_id));
    write_register(fd, 0x820C0024, 0x00000001, next_packet_id(packet_id));
    return 0;
}


//lucid polar -- ok
int gvcp_watchdog_lucid_polar(int fd, uint32_t packet_id) {
#if 0
    uint32_t value = -1;
    read_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value, next_packet_id(packet_id));
    if (value != 0x02) write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    //write_register(fd, 0x10300004, 0x01, next_packet_id(packet_id));
    read_register(fd, GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
#else
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
#endif
    return 0;
}

int gvcp_start_lucid_polar(int fd, uint32_t packet_id) {
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    write_register(fd, 0x10300004, 0x01, next_packet_id(packet_id));
    return 0;
}

int gvcp_stop_lucid_polar(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    read_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value, next_packet_id(packet_id)); 
    write_register(fd, 0x10300004, 0x00, next_packet_id(packet_id));
    return 0;
}

//flir polar -- doesn't work
int gvcp_watchdog_flir_polar(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    read_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value, next_packet_id(packet_id));
    //if (value != 0x02) write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    //read_register(fd, GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
    return 0;
}

int gvcp_start_flir_polar(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
    write_register(fd, 0x00081044, 0x00, next_packet_id(packet_id));
    write_register(fd, 0x00081024, 0x00, next_packet_id(packet_id));
    write_register(fd, 0x00081084, 0xc8, next_packet_id(packet_id));
    write_register(fd, 0x00081064, 0xc8, next_packet_id(packet_id));
/*
    read_register(fd, 0x000c110c, &value, next_packet_id(packet_id));
    read_register(fd, 0x000c1148, &value, next_packet_id(packet_id));
    read_register(fd, 0x000c1108, &value, next_packet_id(packet_id));
    read_register(fd, 0x000c8c40, &value, next_packet_id(packet_id));
    read_register(fd, 0x000c8c20, &value, next_packet_id(packet_id));
    read_register(fd, 0x000c8c00, &value, next_packet_id(packet_id));
    write_register(fd, 0x000c8424, 0x00, next_packet_id(packet_id));
    write_register(fd, 0xc1124, 0x01, next_packet_id(packet_id));
    read_register(fd, 0x000c1148, &value, next_packet_id(packet_id));
    write_register(fd, 0x000c1104, 0xbebc20, next_packet_id(packet_id)); //wtf
    read_register(fd, 0x20002008, &value, next_packet_id(packet_id));
*/

    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
    //write_register(fd, GV_STREAM_CHANNEL_0_PORT_OFFSET, port, next_packet_id(packet_id));
    read_register(fd,  GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
    read_register(fd, GV_STREAM_CHANNEL_0_PACKET_SIZE_OFFSET, &value, next_packet_id(packet_id));

     write_register(fd, 0x00004050, 0, next_packet_id(packet_id));
     write_register(fd, 0x00004054, 0, next_packet_id(packet_id));

    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
        write_register(fd, 0xc00c8, 0x00, next_packet_id(packet_id));
        write_register(fd, 0xc1104, 0x01, next_packet_id(packet_id));
        write_register(fd, 0xc0004, 0x01, next_packet_id(packet_id));
    return 0;
}

int gvcp_stop_flir_polar(int fd, uint32_t packet_id) {
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    write_register(fd, 0xc1124, 0x00, next_packet_id(packet_id)); 
    return 0;
}



//TODO: gvcp_genicam_xml()

//TODO: gvcp_apply_command_sequence()

//ops for error cases
struct gige_camera_ops gige_camera_ops_no = {gvcp_watchdog_no, gvcp_start_no, gvcp_stop_no};

struct gige_camera_ops gige_camera_ops_arvfake = {gvcp_watchdog_arvfake, gvcp_start_arvfake, gvcp_stop_arvfake};
struct gige_camera_ops gige_camera_ops_flir_color = {gvcp_watchdog_flir_color, gvcp_start_flir_color, gvcp_stop_flir_color};
struct gige_camera_ops gige_camera_ops_flir_color2 = {gvcp_watchdog_flir_color2, gvcp_start_flir_color2, gvcp_stop_flir_color2};
struct gige_camera_ops gige_camera_ops_flir_polar = {gvcp_watchdog_flir_polar, gvcp_start_flir_polar, gvcp_stop_flir_polar};
struct gige_camera_ops gige_camera_ops_lucid_polar = {gvcp_watchdog_lucid_polar, gvcp_start_lucid_polar, gvcp_stop_lucid_polar};
struct gige_camera_ops gige_camera_ops_baumer = {gvcp_watchdog_baumer, gvcp_start_baumer, gvcp_stop_baumer};




