#pragma once

void *platform_dlopen(const char *path);
void  platform_dlsym(void **var, void *handle, const char *sym);
void  platform_dlclose(void *handle);
