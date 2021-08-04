#pragma once

#include <shared/math.h>

enum log_type {
    LOG_ERROR = 0,
    LOG_WARNING,
    LOG_INFO
};

typedef void platform_log_func(enum log_type, const char *, ...);
struct platform_function_table {
    platform_log_func *log;
};

struct game_memory {
    struct platform_function_table platform;
};

struct render_commands {
    u8 *memory_base;
    u32 memory_size;
    u32 memory_top;
};

enum render_entry_type {
    ENTRY_TYPE_render_entry_quad,
};

struct render_entry_header {
    u8 type;
};

struct render_entry_quad {
    struct v2 lower_left_corner;
    struct v2 upper_right_corner;
};

#define RENDERER_MEMORY_SIZE 1024

struct renderer {
    struct platform_function_table platform;
    struct render_commands cmds;
    u8 memory[RENDERER_MEMORY_SIZE];
};

static inline void *push_render_entry_impl(struct render_commands *cmds, u32 entry_size, enum render_entry_type type) {
    /* assert(group->top + entry_size <= group->size); */

    struct render_entry_header *header = (struct render_entry_header *)(cmds->memory_base + cmds->memory_top);
    cmds->memory_top += entry_size;

    header->type = (u8) type;

    return header;
}
#define PUSH_RENDER_ENTRY(group, type) \
	(struct type *) push_render_entry_impl(group, sizeof(struct type), ENTRY_TYPE_##type)

/* Function tables */

struct frame_input;
typedef void game_update_func(struct game_memory *, struct frame_input *, struct render_commands *);

struct game_function_table {
    game_update_func *update;
};
static char *game_function_names[] = {
    "update"
};

typedef struct render_commands *renderer_begin_frame_func(struct renderer *);
typedef void renderer_end_frame_func(struct renderer *,struct render_commands *);

struct renderer_function_table {
    renderer_begin_frame_func *begin_frame;
    renderer_end_frame_func *end_frame;
};
static char *renderer_function_names[] = {
    "begin_frame",
    "end_frame"
};
