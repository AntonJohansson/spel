#include "platform.h"

/* libc */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

/* unix */
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>

void platformLog(LogType type, const char *fmt, ...) {
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

/* dl */

void *platformDynamicLibOpen(const char *path) {
    void *handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        platformLog(LOG_ERROR, "Failed to dlopen() with error %s!", dlerror());
        return NULL;
    }
    return handle;
}

void platformDynamicLibLookup(void **var, void *handle, const char *sym) {
    *var = dlsym(handle, sym);
    if (!*var) {
        platformLog(LOG_ERROR, "Failed to dlopen() for sym %s!", sym);
    }
}

void platformDynamicLibClose(void *handle) {
    dlclose(handle);
}

/* mem */

u64 platformMemoryPageSize() {
    return sysconf(_SC_PAGESIZE);
}

void *platformMemoryAllocatePages(u64 num_pages) {
    u64 page_size = platformMemoryPageSize();
    void *ptr = mmap(NULL, num_pages*page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
    if (ptr == MAP_FAILED) {
        platformLog(LOG_ERROR, "Failed to allocate %llu pages (%s)", num_pages,  strerror(errno));
    }
    return ptr;
}

void platformMemoryFreePages(void *ptr, u64 num_pages) {
    u64 page_size = platformMemoryPageSize();
    if (munmap(ptr, num_pages*page_size) != 0) {
        platformLog(LOG_ERROR, "Failed to free %llu pages (%s)", num_pages,  strerror(errno));
    }
}

void *platformMemoryAllocate(u64 size) {
    void *mem = malloc(size);
    if (!mem) {
        platformLog(LOG_ERROR, "Failed to allocate memory of size %lu!", size);
        return NULL;
    }

    return mem;
}

void platformMemoryFree(void *mem) {
    //free(mem);
}

/* file */
static sds file_search_dir;

void platformFileSetSearchDir(sds dir) {
    file_search_dir = dir;
}

File platformFileOpen(const char *path, const char *mode) {
    sds path_in_dir = sdsnew(file_search_dir);
    path_in_dir = sdscat(sdscat(path_in_dir, "/"), path);
    platformLog(LOG_INFO, path_in_dir);
    FILE *fd = fopen(path_in_dir, mode);
    if (!fd) {
        platformLog(LOG_ERROR, "fopen %s (%s)", path_in_dir, strerror(errno));
        sdsfree(path_in_dir);
        return (File) { NULL };
    }
    sdsfree(path_in_dir);

    return (File) { fd };
}

void platformFileClose(File file) {
    if (!file.fd) {
        platformLog(LOG_ERROR, "platform_close called on NULL fd");
        return;
    }

    fclose(file.fd);
}

u64 platformFileSize(File file) {
    if(fseek(file.fd, 0, SEEK_END) == -1) {
        platformLog(LOG_ERROR, "fseek (%s)", strerror(errno));
        platformFileClose(file);
        return 0;
    }

    i64 ret = ftell(file.fd);
    if (ret == -1) {
        platformLog(LOG_ERROR, "ftell (%s)", strerror(errno));
        platformFileClose(file);
        return 0;
    }

    if (fseek(file.fd, 0, SEEK_SET) == -1) {
        platformLog(LOG_ERROR, "fseek (%s)", strerror(errno));
        platformFileClose(file);
        return 0;
    }

    return ret;
}

void platformFileReadToBuffer(const char *path, u8 **buffer, u64 *size) {
    File file = platformFileOpen(path, "r");
    *size = platformFileSize(file);
    *buffer = platformMemoryAllocate(*size + 1);
    (*buffer)[*size] = 0;

    if (fread(*buffer, 1, *size, file.fd) < *size) {
        platformLog(LOG_ERROR, "fread (%s)", strerror(errno));
        platformMemoryFree(*buffer);
        platformFileClose(file);
        return;
    }
    platformFileClose(file);
}

void platformFileWrite(File file, void *ptr, u64 size, u64 amount) {
    if (fwrite(ptr, size, amount, file.fd) != amount) {
        platformLog(LOG_ERROR, "platform_file_write failed to read amount %llu items of size %llu", amount, size);
    }
}

void platformFileRead(File file, void *ptr, u64 size, u64 amount) {
    if (fread(ptr, size, amount, file.fd) != amount) {
        platformLog(LOG_ERROR, "platform_file_read failed to read amount %llu items of size %llu", amount, size);
    }
}

u64 platformFileLastModify(const char *path) {
    struct stat file_info;
    if (stat(path, &file_info) == 0) {
        return (u64) file_info.st_mtim.tv_sec;
    }
    return 0;
}

/* Time */

Time platformTimeCurrent() {
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t) != 0) {
        platformLog(LOG_INFO, "clock_gettime failed!");
    }
    return (Time) {
        .seconds = t.tv_sec,
        .nanoseconds = t.tv_nsec,
    };
}

Time platformTimeSubtract(Time t0, Time t1) {
    return (Time) {
        .seconds = t0.seconds - t1.seconds,
        .nanoseconds = t0.nanoseconds - t1.nanoseconds,
    };
}

u64 platformTimeToNanoseconds(Time t) {
    return t.seconds * 1000000000 + t.nanoseconds;
}

bool platformTimeEarlierThan(Time t0, Time t1) {
    return platformTimeToNanoseconds(t0) < platformTimeToNanoseconds(t1);
}

/* Sleep */
void platformSleepNanoseconds(Time t) {
    struct timespec tspec = {
        .tv_sec = t.seconds,
        .tv_nsec = t.nanoseconds,
    };
    /* Loop in case the sleep gets interrupted */
    while (nanosleep(&tspec, &tspec) != 0) {
    }
}

/* debug */

void platformAbort() {
    abort();
}
