#ifndef _SHARED_MEM_H
#define _SHARED_MEM_H

#ifdef __cplusplus
extern "C" {
#endif

#define SHMEM_NAME_ENV     "SHMEM_DATA_NAME"
#define SHMEM_SIZE_ENV     "SHMEM_DATA_SIZE"

size_t shared_memory_get_size(void *shmem);

void *shared_memory_server_init(const char *shmpath, void *shmmap);
void *shared_memory_client_init(const char *shmpath, size_t shmsize);
void shared_memory_destroy(const char *shmpath, void *shmem);

void shared_memory_unlink(const char *shmpath);

int shared_memory_setenv(const char *shmpath, void *shmem);
int shared_memory_getenv(char **p_shmpath, size_t *shmsize);

// Set @offset, @buffer and @size for r/w into segment
// @Returns pointer to shmem segment from offset
// Usage: ...
void *shared_memory_write_data_segment(void *shmem, size_t offset, char *buffer, size_t size);
void *shared_memory_read_data_segment(void *shmem, size_t offset, char *buffer, size_t size);
void *shared_memory_write_descriptions_segment(void *shmem, size_t offset, char *buffer, size_t size);
void *shared_memory_read_descriptions_segment(void *shmem, size_t offset, char *buffer, size_t size);
void *shared_memory_write_processes_segment(void *shmem, size_t offset, char *buffer, size_t size);
void *shared_memory_read_processes_segment(void *shmem, size_t offset, char *buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif //_SHARED_MEM_H

