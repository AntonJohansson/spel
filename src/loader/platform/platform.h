#pragma once

#include <shared/types.h>
#include <shared/api.h>

void  platform_log(enum log_type type, const char *fmt, ...);
/* dl */
void *platform_dlopen(const char *path);
void  platform_dlsym(void **var, void *handle, const char *sym);
void  platform_dlclose(void *handle);
/* mem */
void *platform_allocate_memory(u64 size);
void  platform_free_memory(void *mem);
/* file */
void  platform_set_search_dir(
u64   platform_last_file_modify(const char *path);
void  platform_read_file_to_buffer(const char *path, u8 *buffer, u64 *size);
/* debug */
void  platform_abort();
