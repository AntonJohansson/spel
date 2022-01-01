#pragma once

#include <shared/math.h>
#include <shared/color.h>

typedef enum LogType {
    LOG_ERROR = 0,
    LOG_WARNING,
    LOG_INFO
} LogType;

typedef void  PlatformLogFunc(LogType, const char *, ...);
typedef void *PlatformMemoryAllocateFunc(u64);
typedef void  PlatformMemoryFreeFunc(void *);
typedef void  PlatformFileReadToBufferFunc(const char *, u8 **, u64 *);
typedef void  PlatformAbortFunc();
typedef struct PlatformFunctionTable {
    PlatformLogFunc *log;
    PlatformMemoryAllocateFunc *allocate_memory;
    PlatformMemoryFreeFunc *free_memory;
    PlatformFileReadToBufferFunc *read_file_to_buffer;
    PlatformAbortFunc *abort;
} PlatformFunctionTable;

typedef struct GameMemory {
    PlatformFunctionTable platform;
    Vec2 pos;
    ColorHSL col;
} GameMemory;

typedef struct RenderCommands {
    u8 *memory_base;
    u32 memory_size;
    u32 memory_top;
} RenderCommands;

typedef enum RenderEntryType {
    ENTRY_TYPE_RenderEntryQuad,
} RenderEntryType;

typedef struct RenderEntryHeader {
    u8 type;
} RenderEntryHeader;

typedef struct RenderEntryQuad {
    RenderEntryHeader header;
    Vec2 pos;
    Vec2 size;
    ColorRGB col;
} RenderEntryQuad;

#define RENDERER_MEMORY_SIZE 1024

struct GLFWwindow;
typedef struct GLFWwindow GLFWwindow;

typedef struct RenderContext RenderContext;

typedef struct Renderer {
    GLFWwindow *window;
    RenderContext *context;
    PlatformFunctionTable platform;
    RenderCommands cmds;
    u8 memory[RENDERER_MEMORY_SIZE];
} Renderer;

static inline void *push_render_entry_impl(RenderCommands *cmds, u32 entry_size, RenderEntryType type) {
    RenderEntryHeader *header = (RenderEntryHeader *)(cmds->memory_base + cmds->memory_top);
    cmds->memory_top += entry_size;

    header->type = (u8) type;

    return header;
}

#define PUSH_RENDER_ENTRY(group, Type) \
    (Type *) push_render_entry_impl(group, sizeof(Type), ENTRY_TYPE_##Type)

static inline void pushQuad(RenderCommands *cmds, Vec2 pos, Vec2 size, ColorRGB col) {
    RenderEntryQuad *quad = PUSH_RENDER_ENTRY(cmds, RenderEntryQuad);
    quad->pos = pos;
    quad->size = size;
    quad->col = col;
}
