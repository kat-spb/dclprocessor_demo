
#include "env_manipulations.h"

extern "C" {
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "shmem/shared_memory.h"
#include "shmem/memory_map.h"
}

namespace tmk {
    void storeAndCleanEnv(char **path_old, char **size_old) {
        char *path = getenv(SHMEM_NAME_ENV);
        if (path && path_old) {
            *path_old = strdup(path);
            unsetenv(SHMEM_NAME_ENV);
        }
        char *size = getenv(SHMEM_SIZE_ENV);
        if (size && size_old) {
            *size_old = strdup(size);
            unsetenv(SHMEM_SIZE_ENV);
        }
    }

    void restoreEnv(char **path_old, char **size_old) {
        if (path_old && *path_old) {
            setenv(SHMEM_NAME_ENV, *path_old, 1);
            free(*path_old);
        } else {
            unsetenv(SHMEM_NAME_ENV);
        }
        if (size_old && *size_old) {
            setenv(SHMEM_SIZE_ENV, *size_old, 1);
            free(*size_old);
        } else {
            unsetenv(SHMEM_SIZE_ENV);
        }
    }
};
