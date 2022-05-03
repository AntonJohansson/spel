
/* external */
#include <GLFW/glfw3.h>

#include <ft2build.h>
#include FT_FREETYPE_H

/* shared */
#include <shared/types.h>
#include <shared/api.h>
#include <shared/input.h>

/* libc */
#include <stdbool.h>
#include <string.h>

#include "platform/unix.c"

/*
 * Function tables
 */

/* Game */
typedef void GameUpdateFunc(f32, GameMemory *, Input *, RenderCommands *);

typedef struct GameFunctionTable {
    GameUpdateFunc *update;
} GameFunctionTable;

static char *game_function_names[] = {
    "update"
};

/* Debug */
typedef void DebugPreUpdateFunc(f32, DebugMemory *, Input *, RenderCommands *);
typedef void DebugPostUpdateFunc(f32, DebugMemory *, Input *, RenderCommands *);

typedef struct DebugFunctionTable {
    DebugPreUpdateFunc  *pre_update;
    DebugPostUpdateFunc *post_update;
} DebugFunctionTable;

static char *debug_function_names[] = {
    "pre_update",
    "post_update"
};

/* Renderer */
typedef void RendererStartupFunc(Renderer *);
typedef void RendererShutdownFunc(Renderer *);
typedef RenderCommands *RendererBeginFrameFunc(Renderer *);
typedef void RendererEndFrameFunc(Renderer *, RenderCommands *);

typedef struct RendererFunctionTable {
    RendererStartupFunc     *startup;
    RendererShutdownFunc    *shutdown;
    RendererBeginFrameFunc  *begin_frame;
    RendererEndFrameFunc    *end_frame;
} RendererFunctionTable;

static char *renderer_function_names[] = {
    "startup",
    "shutdown",
    "begin_frame",
    "end_frame"
};

/*
 * Frame input
 */

static Input global_frame_input = {0};
static Input global_debug_frame_input = {0};

static InputType global_key_map[GLFW_KEY_LAST] = {
    [GLFW_KEY_W] = INPUT_MOVE_UP,
    [GLFW_KEY_A] = INPUT_MOVE_LEFT,
    [GLFW_KEY_S] = INPUT_MOVE_DOWN,
    [GLFW_KEY_D] = INPUT_MOVE_RIGHT,
};

static DebugInputType global_debug_key_map[GLFW_KEY_LAST] = {
    [GLFW_KEY_1] = DEBUG_INPUT_RECORD_START,
    [GLFW_KEY_2] = DEBUG_INPUT_RECORD_STOP,
    [GLFW_KEY_3] = DEBUG_INPUT_REPLAY_START,
    [GLFW_KEY_4] = DEBUG_INPUT_REPLAY_STOP,
};

#include "glfw_input.c"

/*
 * Code modules
 */

typedef struct CodeModule {
    const char *path;
    void *handle;

    u64 last_modify_time;

    u8 function_count;
    void **functions;
    char **function_names;
} CodeModule;

static inline void loadCodeModule(CodeModule *module) {
    module->handle = platformDynamicLibOpen(module->path);
    if (!module->handle) {
        platformLog(LOG_ERROR, "Failed to load module %s!", module->path);
        return;
    }

    for (u8 i = 0; i < module->function_count; ++i) {
        platformDynamicLibLookup(&module->functions[i], module->handle, module->function_names[i]);
    }
}

static inline void unloadCodeModule(CodeModule *module) {
    platformDynamicLibClose(module->handle);
}

static inline void reloadCodeModuleIfNeeded(CodeModule *module) {
    u64 new_modify_time = platformFileLastModify(module->path);
    if (new_modify_time > module->last_modify_time) {
        sleep(1);
        platformLog(LOG_INFO, "Reloading module %s", module->path);
        module->last_modify_time = new_modify_time;
        unloadCodeModule(module);
        loadCodeModule(module);
    }
}

/*
 * Frame
 */

static inline void beginFrame(FrameInfo *frame_info) {
    frame_info->start_time = platformTimeCurrent();
}

static inline void endFrame(FrameInfo *frame_info) {
    frame_info->end_time = platformTimeCurrent();
    frame_info->elapsed_time = platformTimeSubtract(frame_info->end_time, frame_info->start_time);

    if (platformTimeEarlierThan(frame_info->elapsed_time, frame_info->desired_time)) {
        frame_info->elapsed_time = platformTimeSubtract(frame_info->desired_time, frame_info->elapsed_time);
        platformSleepNanoseconds(frame_info->elapsed_time);
    }

    frame_info->total_frame_count++;
}

static void load_font_atlas(FT_Face face, uint32_t font_size, Image *image, PackRect **font_map, FontInfo **font_info) {

    *font_map = platformMemoryAllocate(NUM_CHARS*sizeof(PackRect));
    *font_info = platformMemoryAllocate(NUM_CHARS*sizeof(FontInfo));

    /* Load font rects */
    for (u8 i = 0; i < NUM_CHARS; ++i) {
        FT_UInt glyph_index = FT_Get_Char_Index(face, i);
        if (FT_Load_Char(face, glyph_index, FT_LOAD_BITMAP_METRICS_ONLY)) {
            platformLog(LOG_ERROR, "FreeType: Could not load glyph for character: %c", i);
            return;
        }

        (*font_map)[i].width = face->glyph->bitmap.width;
        (*font_map)[i].height = face->glyph->bitmap.rows;
        (*font_map)[i].user_id = glyph_index;

        (*font_info)[i].codepoint = glyph_index;
        (*font_info)[i].advance   = face->glyph->advance.x;
        (*font_info)[i].offset_x  = face->glyph->bitmap_left;
        (*font_info)[i].offset_y  = face->glyph->bitmap_top;
    }

    const u32 pack_width = sqrt(font_size*font_size*NUM_CHARS);
    const u32 pack_height = pack_width;
    PackNode nodes[pack_width+2];
    PackContext pack_context = {
        .width = pack_width,
        .height = pack_height,
        .num_nodes = pack_width + 2,
        .nodes = nodes,
    };
    pack_rectangles(&pack_context, *font_map, NUM_CHARS);

    u8 pack_pixels[pack_width*pack_height];
    memset(pack_pixels, 0, pack_width*pack_height*sizeof(u8));
    for (u8 i = 0; i < NUM_CHARS; ++i) {
        u8 index = (*font_map)[i].user_id;
        if (FT_Load_Char(face, index, FT_LOAD_RENDER)) {
            platformLog(LOG_ERROR, "FreeType: Could not load glyph for character: %c", i);
            return;
        }
        if ((*font_map)[i].width > 0 && (*font_map)[i].height > 0) {
            for (u32 y = 0; y < (*font_map)[i].height; ++y) {
                for (u32 x = 0; x < (*font_map)[i].width; ++x) {
                    pack_pixels[((*font_map)[i].y + y)*pack_width + (*font_map)[i].x + x] = face->glyph->bitmap.buffer[y*(*font_map)[i].width + x];
                }
            }
        }
    }

    image->pixels = platformMemoryAllocate(pack_width*pack_height*sizeof(u8));
    image->width = pack_width;
    image->height = pack_height;
    image->channels = 1;

    memcpy(image->pixels, pack_pixels, pack_width*pack_height*sizeof(u8));
}

/*
 * Main
 */

int main(int argc, char **argv) {
    (void)argc;

    sds dir = sdsnew(argv[0]);
    /* Remove the executable name */
    {
        size_t i = sdslen(dir);
        for (; i > 0; --i) {
            if (dir[i] == '/') {
                break;
            }
        }
        dir[i] = 0;
    }
    sdsupdatelen(dir);
    platformFileSetSearchDir(dir);

    sds game_path = sdsnew(dir);
    game_path = sdscat(game_path, "/libgame");

    sds debug_path = sdsnew(dir);
    debug_path = sdscat(debug_path, "/libdebug");

    sds renderer_path = sdsnew(dir);
    renderer_path = sdscat(renderer_path, "/librenderer");

    /* Frame info */
    FrameInfo frame_info = {
        .total_frame_count = 0,
        .start_time = platformTimeCurrent(),
        .end_time = {},
        .elapsed_time = {},
        .desired_time = {
            .seconds = 0,
            .nanoseconds = (1.0f/60.0f) * 1000000000.0f,
        },
    };

    /* Code Module */
    GameFunctionTable game_functions = {0};
    CodeModule game_module = {
        .path = game_path,
        .last_modify_time = platformFileLastModify(game_path),
        .function_count = ARRLEN(game_function_names),
        .functions = (void **) &game_functions,
        .function_names = game_function_names,
    };
    loadCodeModule(&game_module);

    DebugFunctionTable debug_functions = {0};
    CodeModule debug_module = {
        .path = debug_path,
        .last_modify_time = platformFileLastModify(debug_path),
        .function_count = ARRLEN(debug_function_names),
        .functions = (void **) &debug_functions,
        .function_names = debug_function_names,
    };
    loadCodeModule(&debug_module);

    RendererFunctionTable renderer_functions = {0};
    CodeModule renderer_module = {
        .path = renderer_path,
        .last_modify_time = platformFileLastModify(renderer_path),
        .function_count = ARRLEN(renderer_function_names),
        .functions = (void **) &renderer_functions,
        .function_names = renderer_function_names,
    };
    loadCodeModule(&renderer_module);

    PlatformFunctionTable platform_functions = {
        .log = platformLog,

        .allocate_memory = platformMemoryAllocate,
        .free_memory = platformMemoryFree,

        .file_open = platformFileOpen,
        .file_close = platformFileClose,
        .file_size = platformFileSize,
        .read_file_to_buffer = platformFileReadToBuffer,
        .file_write = platformFileWrite,
        .file_read = platformFileRead,

        .abort = platformAbort,
    };

    GameMemory game_memory = {
        .platform = platform_functions,
        .frame_info = &frame_info,
    };

    DebugMemory debug_memory = {
        .platform = platform_functions,
        .frame_info = &frame_info,
        .active_game_memory = &game_memory,
        .active_game_input = &global_frame_input,
    };

    Renderer renderer = {
        .platform = platform_functions,
        .frame_info = &frame_info,
    };

    RenderCommands *frame = NULL;

    /* Load font */

    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        platformLog(LOG_ERROR, "FreeType: Could not init!");
        return 1;
    }

    u8 *font = NULL;
    u64 file_size = 0;
    platformFileReadToBuffer("res/fonts/lmmono12-regular.otf", &font, &file_size);

    FT_Open_Args args = {
        .flags = FT_OPEN_MEMORY,
        .memory_base = font,
        .memory_size = file_size,
        .pathname = NULL,
        .stream = 0,
        .driver = NULL,
        .num_params = 0,
        .params = NULL,
    };

    FT_Face face;
    if (FT_Open_Face(ft, &args, 0, &face)) {
        platformLog(LOG_ERROR, "FreeType: Could not open face!");
        return 2;
    }

    platformMemoryFree(font);

    uint32_t font_size = 60;
    FT_Set_Pixel_Sizes(face, 0, font_size);

    Image font_atlas;
    PackRect *font_map;
    FontInfo *font_info;
    load_font_atlas(face, font_size, &font_atlas, &font_map, &font_info);
    assert(font_map);

    renderer.font_map = font_map;
    renderer.font_atlas = &font_atlas;
    renderer.font_info = font_info;

    /* glfw init */

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, false);
    renderer.window = glfwCreateWindow(800, 600, "spel", 0, 0);
    glfwMakeContextCurrent(renderer.window);

    set_glfw_input_callbacks(renderer.window);

    /* startup modules */

    if (renderer_functions.startup) {
        renderer_functions.startup(&renderer);
    }

    while (!glfwWindowShouldClose(renderer.window)) {
        beginFrame(&frame_info);
        glfwPollEvents();

        /* Call out to game modules */
        if (renderer_functions.begin_frame) {
            frame = renderer_functions.begin_frame(&renderer);
        }
        if (debug_functions.pre_update) {
            debug_functions.pre_update((1.0f/60.0f), &debug_memory, &global_debug_frame_input, frame);
        }
        if (game_functions.update) {
            game_functions.update((1.0f/60.0f), &game_memory, &global_frame_input, frame);
        }
        if (debug_functions.post_update) {
            debug_functions.post_update((1.0f/60.0f), &debug_memory, &global_debug_frame_input, frame);
        }
        if (renderer_functions.end_frame) {
            renderer_functions.end_frame(&renderer, frame);
        }

        reloadCodeModuleIfNeeded(&debug_module);
        reloadCodeModuleIfNeeded(&game_module);
        reloadCodeModuleIfNeeded(&renderer_module);

        endFrame(&frame_info);
    }

    if (renderer_functions.shutdown) {
        renderer_functions.shutdown(&renderer);
    }

    platformMemoryFree(font_map);
    platformMemoryFree(font_atlas.pixels);

    glfwDestroyWindow(renderer.window);
    glfwTerminate();

    unloadCodeModule(&debug_module);
    unloadCodeModule(&game_module);
    unloadCodeModule(&renderer_module);

    sdsfree(game_path);
    sdsfree(renderer_path);
    sdsfree(dir);

    return 0;
}
