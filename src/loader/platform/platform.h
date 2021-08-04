#pragma once

#include <shared/types.h>
#include <shared/api.h>

void  platform_log(enum log_type type, const char *fmt, ...);
void *platform_dlopen(const char *path);
void  platform_dlsym(void **var, void *handle, const char *sym);
void  platform_dlclose(void *handle);
u64   platform_last_file_modify(const char *path);
