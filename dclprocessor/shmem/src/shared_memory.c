#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/mman.h>
#include <fcntl.h>

//#include <misc/log.h>
#include <shmem/memory_map.h>
#include "shmem/shared_memory.h"

void *shared_memory_server_init(const char *shmpath, void *shmmap) {
    if (!shmpath) {
        fprintf(stderr, "[%s]: couldn't init shared memory without shmpath\n", __FUNCTION__);
        return NULL;
    }
    if (!shmmap) {
        fprintf(stderr, "[%s]: couldn't init shared memory without memory map\n", __FUNCTION__);
    }
    memory_map_t *map = (memory_map_t*)shmmap;
    int fd = shm_open(shmpath, O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
    if (fd == -1) {
        fprintf(stderr, "[%s]: can't create shared memory\n", __FUNCTION__);
        return NULL;
    }
    if (ftruncate(fd, memory_get_size(map)) == -1) {
        fprintf(stderr, "[%s]: can't set shared memory size\n", __FUNCTION__);
        shm_unlink(shmpath);
        close(fd);
        return NULL;
    }
    char *shmdata = mmap(NULL, memory_get_size(map),
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED, fd, 0);
    if (shmdata == MAP_FAILED) {
        fprintf(stderr, "[%s]: mmap error\n", __FUNCTION__);
        shm_unlink(shmpath);
        close(fd);
        return NULL;
    }

    memcpy(shmdata, map, sizeof(memory_map_t));
    //shared_memory_setenv(shmpath, shmdata);
    close(fd);
    return shmdata;
}

//TODO: think about usage of get env
void *shared_memory_client_init(const char *shmpath, size_t shmsize) {
    int fd = shm_open(shmpath, O_RDWR, 0);
    if (fd == -1) {
        fprintf(stderr, "[shared_memory, client]: can't open shared memory\n");
        return NULL;
    }
    char *shmdata = mmap(NULL, shmsize,
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED, fd, 0);
    if (shmdata == MAP_FAILED) {
        fprintf(stderr, "[shared_memory, client]: mmap error\n");
        shm_unlink(shmpath);
        close(fd);
        return NULL;
    }
    close(fd);
    return shmdata;
}

size_t shared_memory_get_size(void *shmem) {
    memory_map_t *map = (memory_map_t*)shmem;
    return memory_get_size(map);
}

int shared_memory_setenv(const char *shmpath, void *shmem) {
    if (!shmpath) return -1;
    memory_map_t *map = (memory_map_t *)shmem;
    size_t shm_size = memory_get_size(map);
    char shm_size_str[40];
    //TODO: thinks about move SHMEM_NAME_ENV to config -- O.Zh.
    if (setenv(SHMEM_NAME_ENV, shmpath, 1) != 0){
        fprintf(stderr, "[shared_memory, server]: can't set %s value for child process: errno=%d, %s\n", 
            SHMEM_SIZE_ENV, errno, strerror(errno));
        fflush(stderr);
        return -1;
    }

    snprintf(shm_size_str, sizeof(shm_size_str), "%zd", shm_size);
    if (setenv(SHMEM_SIZE_ENV, shm_size_str, 1) != 0){
        fprintf(stderr, "[shared_memory, server]: can't set %s value for child process: errno=%d, %s\n",
            SHMEM_SIZE_ENV, errno, strerror(errno));
        fflush(stderr);
        return -1;
    }
    
    return 0;
}

int shared_memory_getenv(char **p_shmpath, size_t *shmsize) {
    char *shm_size_str = getenv(SHMEM_SIZE_ENV);
    char *shmpath = *p_shmpath;
    if (shmpath != NULL) {
        //LOG_FD_INFO("Shared memory path will discarded\n");
        fprintf(stdout, "Shared memory path will discarded\n");
        fflush(stdout);
        free(shmpath);
    }
    shmpath = strdup(getenv(SHMEM_NAME_ENV));
    
    if (shmpath == NULL){
        fprintf(stderr, "[shared_memory, client]: %s is not set\n",
            SHMEM_NAME_ENV);
        fflush(stderr);
        return -1;
    }
    if (shm_size_str == NULL){
        fprintf(stderr, "[shared_memory, client]: %s is not set\n",
            SHMEM_SIZE_ENV);
        fflush(stderr);
        return -1;
    }

    sscanf(shm_size_str, "%zu", shmsize);
    *p_shmpath = shmpath;
    return 0;
}

void *shared_memory_write_data_segment(void *shmem, size_t offset, char *buffer, size_t size){
    size_t data_offset = memory_get_data_offset(shmem);
    char *data = (char*)shmem + data_offset + offset;
    if (buffer && size > 0) memcpy(data, buffer, size);
    return shmem + data_offset + offset;
}

void *shared_memory_read_data_segment(void *shmem, size_t offset, char *buffer, size_t size) {
    size_t data_offset = memory_get_data_offset(shmem);
    char *data = (char*)shmem + data_offset + offset;
    if (buffer && size > 0) memcpy(buffer, data, size);
    return shmem + data_offset + offset;
}

void *shared_memory_write_descriptions_segment(void *shmem, size_t offset, char *buffer, size_t size){
    size_t description_offset = memory_get_description_offset(shmem);
    char *data = (char*)shmem + description_offset + offset;
    if (buffer && size > 0) memcpy(data, buffer, size);
    return shmem + description_offset + offset;
}

void *shared_memory_read_descriptions_segment(void *shmem, size_t offset, char *buffer, size_t size) {
    size_t description_offset = memory_get_description_offset(shmem);
    char *data = (char*)shmem + description_offset + offset;
    if (buffer && size > 0) memcpy(buffer, data, size);
    return shmem + description_offset + offset;
}

void *shared_memory_write_processes_segment(void *shmem, size_t offset, char *buffer, size_t size) {
    size_t process_offset = memory_get_process_offset(shmem);
    char *data = (char*)shmem + process_offset + offset;
    if (buffer && size > 0) memcpy(data, buffer, size);
    return shmem + process_offset + offset;
}

void *shared_memory_read_processes_segment(void *shmem, size_t offset, char *buffer, size_t size) {
    size_t process_offset = memory_get_process_offset(shmem);
    char *data = (char*)shmem + process_offset + offset;
    if (buffer && size > 0) memcpy(buffer, data, size);
    return shmem + process_offset + offset;
}

void shared_memory_unlink(const char *shmpath) {
    shm_unlink(shmpath);
}

void shared_memory_destroy(const char *shmpath, void *shmem) {
    munmap(shmem, memory_get_size(shmem));
    shm_unlink(shmpath);
}

