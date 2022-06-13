#pragma once

#include <shared/input.h>
#include <shared/math.h>
#include <shared/color.h>
#include <shared/pack_rectangles.h>
#include <third_party/sds.h>

//#include <ft2build.h>
//#include FT_FREETYPE_H

#define NUM_CHARS 128

typedef struct File {
    void *fd;
} File;

typedef struct Time {
    u64 seconds;
    u64 nanoseconds;
} Time;

typedef struct FrameInfo {
    u64 total_frame_count;
    Time start_time;
    Time end_time;
    Time elapsed_time;
    Time desired_time;
} FrameInfo;

typedef enum LogType {
    LOG_ERROR = 0,
    LOG_WARNING,
    LOG_INFO
} LogType;

typedef struct Image {
    u8 *pixels;
    u32 width;
    u32 height;
    u32 channels;
} Image;

typedef struct FontInfo {
    uint8_t codepoint;
    uint32_t  advance;
    uint32_t  offset_x;
    uint32_t  offset_y;
} FontInfo;

typedef void  PlatformLogFunc(LogType, const char *, ...);
typedef void *PlatformMemoryAllocateFunc(u64);
typedef void  PlatformMemoryFreeFunc(void *);

typedef File  PlatformFileOpenFunc(const char *path, const char *mode);
typedef void  PlatformFileCloseFunc(File file);
typedef u64   PlatformFileSizeFunc(File file);
typedef void  PlatformFileReadToBufferFunc(const char *, u8 **, u64 *);
typedef void  PlatformFileWriteFunc(File file, void *ptr, u64 size, u64 amount);
typedef void  PlatformFileReadFunc(File file, void *ptr, u64 size, u64 amount);

typedef void  PlatformAbortFunc();

typedef struct PlatformFunctionTable {
    PlatformLogFunc *log;

    PlatformMemoryAllocateFunc *allocate_memory;
    PlatformMemoryFreeFunc *free_memory;

    PlatformFileOpenFunc *file_open;
    PlatformFileCloseFunc *file_close;
    PlatformFileSizeFunc *file_size;
    PlatformFileReadToBufferFunc *read_file_to_buffer;
    PlatformFileWriteFunc *file_write;
    PlatformFileReadFunc *file_read;

    PlatformAbortFunc *abort;
} PlatformFunctionTable;

typedef struct GameMemory {
    PlatformFunctionTable platform;
    FrameInfo *frame_info;

    Vec2 pos;
    ColorHSL col;
} GameMemory;

/*
 * Debug
 */

typedef enum RecordState {
    IDLE,
    RECORDING,
    REPLAYING,
} RecordState;

typedef struct RecordData {
    Input input;
    GameMemory game_memory;
} RecordData;

typedef struct DebugMemory {
    PlatformFunctionTable platform;
    FrameInfo *frame_info;

    RecordState record_state;
    File record_file;
    bool is_record_file_open;

    u64 replay_index;
    u64 replay_count;

    RecordData *replay_memory;

    GameMemory *active_game_memory;
    Input *active_game_input;

    GameMemory replay_old_memory;
    Input replay_old_input;
} DebugMemory;

typedef struct RenderCommands {
    u8 *memory_base;
    u32 memory_size;
    u32 memory_top;
} RenderCommands;

typedef enum RenderEntryType {
    ENTRY_TYPE_RenderEntryQuad,
    ENTRY_TYPE_RenderEntryTexturedQuad,
    ENTRY_TYPE_RenderEntryAtlasQuad,
    ENTRY_TYPE_RenderEntryText,
} RenderEntryType;

typedef struct RenderEntryHeader {
    u8 type;
} RenderEntryHeader;

typedef struct RenderEntryQuad {
    RenderEntryHeader header;
    Vec2 pos;
    Vec2 scale;
    ColorRGB col;
} RenderEntryQuad;

typedef struct RenderEntryTexturedQuad {
    RenderEntryHeader header;
    Vec2 pos;
    Vec2 scale;
    Image image;
} RenderEntryTexturedQuad;

typedef struct RenderEntryAtlasQuad {
    RenderEntryHeader header;
    Vec2 pos;
    Vec2 scale;
    Image image;
    Vec2 offset;
    Vec2 size;
    ColorRGB col;
} RenderEntryAtlasQuad;

typedef struct RenderEntryText {
    RenderEntryHeader header;
    Vec2 pos;
    ColorRGB col;
    const char *text;
} RenderEntryText;

#define RENDERER_MEMORY_SIZE 1024

struct GLFWwindow;
typedef struct GLFWwindow GLFWwindow;

typedef struct RenderContext RenderContext;

typedef struct Renderer {
    PlatformFunctionTable platform;
    FrameInfo *frame_info;

    GLFWwindow *window;
    RenderContext *context;
    RenderCommands cmds;

    FontInfo *font_info;
    PackRect *font_map;
    Image    *font_atlas;

    u8 memory[RENDERER_MEMORY_SIZE];
} Renderer;

/* Render API */

static inline void *push_render_entry_impl(RenderCommands *cmds, u32 entry_size, RenderEntryType type) {
    RenderEntryHeader *header = (RenderEntryHeader *)(cmds->memory_base + cmds->memory_top);
    cmds->memory_top += entry_size;

    header->type = (u8) type;

    return header;
}

#define PUSH_RENDER_ENTRY(group, Type) \
    (Type *) push_render_entry_impl(group, sizeof(Type), ENTRY_TYPE_##Type)

static inline void pushQuad(RenderCommands *cmds, Vec2 pos, Vec2 scale, ColorRGB col) {
    RenderEntryQuad *quad = PUSH_RENDER_ENTRY(cmds, RenderEntryQuad);
    quad->pos = v2Add(pos, v2Scale(0.5f, scale));
    quad->scale = scale;
    quad->col = col;
}

static inline void pushTexturedQuad(RenderCommands *cmds, Vec2 pos, Vec2 scale, Image image) {
    RenderEntryTexturedQuad *quad = PUSH_RENDER_ENTRY(cmds, RenderEntryTexturedQuad);
    quad->pos = v2Add(pos, v2Scale(0.5f, scale));
    quad->scale = scale;
    quad->image = image;
}

static inline void pushAtlasQuad(RenderCommands *cmds, Vec2 pos, Vec2 scale, Image image, Vec2 offset, Vec2 size, ColorRGB col) {
    RenderEntryAtlasQuad *quad = PUSH_RENDER_ENTRY(cmds, RenderEntryAtlasQuad);
    quad->pos = v2Add(pos, v2Scale(0.5f, scale));
    quad->scale = scale;
    quad->image = image;
    quad->offset = offset;
    quad->size = size;
    quad->col = col;
}

static inline void pushText(RenderCommands *cmds, Vec2 pos, ColorRGB col, const char *text) {
    RenderEntryText *entry = PUSH_RENDER_ENTRY(cmds, RenderEntryText);
    entry->pos  = pos;
    entry->text = text;
    entry->col = col;
}

static inline void pushTextFmt(RenderCommands *cmds, Vec2 pos, ColorRGB col, const char *fmt, ...) {
    //RenderEntryText *entry = PUSH_RENDER_ENTRY(cmds, RenderEntryText);
    //entry->pos  = pos;
    //entry->text = text;
    //entry->col = col;

    //sds text;
    //sds sdscatfmt(sds s, char const *fmt, ...);

    //va_list args;
    //va_start(args, fmt);
    //vfprintf(fd, fmt, args);
    //va_end(args);
}
