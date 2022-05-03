#pragma once

#include <shared/types.h>
#include <shared/api.h>

void platformLog(LogType type, const char *fmt, ...);

/* Dynamic library */
void *platformDynamicLibOpen(const char *path);
void  platformDynamicLibLookup(void **var, void *handle, const char *sym);
void  platformDynamicLibClose(void *handle);

/* Memory */
u64   platformMemoryPageSize();
void *platformMemoryAllocatePages(u64 num_pages);
void  platformMemoryFreePages(void *ptr, u64 num_pages);
void *platformMemoryAllocate(u64 size);
void  platformMemoryFree(void *mem);

void  platformFileSetSearchDir(sds dir);
File  platformFileOpen(const char *path, const char *mode);
void  platformFileClose(File file);
u64   platformFileSize(File file);
void  platformFileReadToBuffer(const char *path, u8 **buffer, u64 *size);
void  platformFileWrite(File file, void *ptr, u64 size, u64 amount);
void  platformFileRead(File file, void *ptr, u64 size, u64 amount);
u64   platformFileLastModify(const char *path);

Time platformTimeCurrent();
Time platformTimeSubtract(Time t0, Time t1);
u64  platformTimeToNanoseconds(Time t);
bool platformTimeEarlierThan(Time t0, Time t1);

/* Sleep */
void platformSleepNanoseconds(Time t);

/* debug */
void  platformAbort();
