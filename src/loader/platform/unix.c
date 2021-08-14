#include "platform.h"

/* libc */
#include <stdio.h>
#include <stdlib.h>

/* unix */
#include <dlfcn.h>
#include <sys/stat.h>
#include <stdarg.h>

void platform_log(enum log_type type, const char *fmt, ...) {
    FILE *fd;
    switch (type) {
        case LOG_ERROR: {
            fd = stderr;
            fprintf(stderr, "\033[31;1m[error]\033[0m ");
            break;
        }
        case LOG_WARNING: {
            fd = stderr;
            fprintf(stderr, "\033[33;1m[warning]\033[0m ");
            break;
        }
        case LOG_INFO: {
            fd = stdout;
            fprintf(stderr, "\033[36;1m[info]\033[0m ");
            break;
        }
        default: {
            fd = stdout;
            fprintf(stderr, "\033[34;1m[????]\033[0m ");
            break;
        }
    };

    va_list args;
    va_start(args, fmt);
    vfprintf(fd, fmt, args);
    va_end(args);

    fputc('\n', fd);
}

void *platform_dlopen(const char *path) {
    void *handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        platform_log(LOG_ERROR, "Failed to dlopen() with error %s!", dlerror());
        return NULL;
    }
    return handle;
}

void platform_dlsym(void **var, void *handle, const char *sym) {
    *var = dlsym(handle, sym);
    if (!*var) {
        platform_log(LOG_ERROR, "Failed to dlopen() for sym %s!", sym);
    }
}

void platform_dlclose(void *handle) {
    dlclose(handle);
}

u64 platform_last_file_modify(const char *path) {
    struct stat file_info;
    if (stat(path, &file_info) == 0) {
        return (u64) file_info.st_mtim.tv_sec;
    }
    return 0;
}

void *platform_allocate_memory(u64 size) {
    void *mem = malloc(size);
    if (!mem) {
        platform_log(LOG_ERROR, "Failed to allocate memory of size %lu!", size);
        return NULL;
    }

    return mem;
}

void platform_free_memory(void *mem) {
    free(mem);
}
