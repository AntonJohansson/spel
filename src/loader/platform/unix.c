#include <stdio.h>
#include <dlfcn.h>

void *platform_dlopen(const char *path) {
    void *handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "Failed to dlopen() for path %s\n!", path);
        return NULL;
    }
    return handle;
}

void platform_dlsym(void **var, void *handle, const char *sym) {
    *var = dlsym(handle, sym);
    if (!*var) {
        fprintf(stderr, "Failed to dlsym() for sym %s\n!", sym);
    }
}

void platform_dlclose(void *handle) {
    dlclose(handle);
}
