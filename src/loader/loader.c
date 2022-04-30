/* external */
#include <GLFW/glfw3.h>
#include "third_party/sds.c"

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

typedef struct Input Input;
typedef void GameUpdateFunc(f32, GameMemory *, Input *, RenderCommands *);

typedef struct GameFunctionTable {
    GameUpdateFunc *update;
} GameFunctionTable;
static char *game_function_names[] = {
    "update"
};

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

/* Debug */

typedef enum RecordState {
    IDLE,
    RECORDING,
    REPLAYING,
} RecordState;

typedef struct RecordData {
    Input input;
    GameMemory game_memory;
} RecordData;

typedef struct DebugState {
    RecordState record_state;
    File record_file;
    bool is_record_file_open;

    u64 replay_index;
    u64 replay_count;

    RecordData *replay_memory;

    GameMemory replay_old_memory;
    Input replay_old_input;
} DebugState;

/* Frame */
typedef struct FrameInfo {
    u64 total_frame_count;
    Time start_time;
    Time end_time;
    Time elapsed_time;
    Time desired_time;
} FrameInfo;

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

/* Main */
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

    sds renderer_path = sdsnew(dir);
    renderer_path = sdscat(renderer_path, "/librenderer");

    /* Debug */
    DebugState debug_state = {0};

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
        .read_file_to_buffer = platformFileReadToBuffer,
        .abort = platformAbort,
    };

    GameMemory game_memory = {
        .platform = platform_functions,
    };


    Renderer renderer = {
        .platform = platform_functions,
    };

    RenderCommands *frame = NULL;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, false);
    renderer.window = glfwCreateWindow(800, 600, "spel", 0, 0);
    glfwMakeContextCurrent(renderer.window);

    set_glfw_input_callbacks(renderer.window);

    if (renderer_functions.startup) {
        renderer_functions.startup(&renderer);
    }

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

    platformLog(LOG_INFO, "Desired frame time: %llu ns", frame_info.desired_time.nanoseconds);

    while (!glfwWindowShouldClose(renderer.window)) {
        beginFrame(&frame_info);
        glfwPollEvents();

        /* Recording input */
        if (global_debug_frame_input.active[DEBUG_INPUT_RECORD_START]) {
            if (debug_state.record_state == IDLE) {
                platformLog(LOG_INFO, "Starting recording");
                debug_state.record_state = RECORDING;
            }
        }
        if (global_debug_frame_input.active[DEBUG_INPUT_RECORD_STOP]) {
            if (debug_state.record_state == RECORDING) {
                platformLog(LOG_INFO, "Stopping recording");
                debug_state.record_state = IDLE;
            }
        }
        if (global_debug_frame_input.active[DEBUG_INPUT_REPLAY_START]) {
            if (debug_state.record_state == IDLE) {
                platformLog(LOG_INFO, "Starting replay");
                debug_state.record_state = REPLAYING;
            }
        }
        if (global_debug_frame_input.active[DEBUG_INPUT_REPLAY_STOP]) {
            if (debug_state.record_state == REPLAYING) {
                platformLog(LOG_INFO, "Stopping replay");
                debug_state.record_state = IDLE;
                memcpy(&game_memory, &debug_state.replay_old_memory, sizeof(GameMemory));
                memcpy(&global_frame_input, &debug_state.replay_old_input, sizeof(Input));
            }
        }

        /* Handle recording of input */
        if (!debug_state.is_record_file_open && (debug_state.record_state == RECORDING || debug_state.record_state == REPLAYING)) {
            const char *mode = (debug_state.record_state == RECORDING) ? "w" : "r";
            debug_state.record_file = platformFileOpen("input_recording", mode);
            debug_state.is_record_file_open = true;
            platformLog(LOG_INFO, "opening fd");
        }
        if (debug_state.record_state == RECORDING) {
            RecordData r;
            memcpy(&r.input, &global_frame_input, sizeof(Input));
            memcpy(&r.game_memory, &game_memory, sizeof(GameMemory));
            platformFileWrite(debug_state.record_file, &r, sizeof(RecordData), 1);
        } else if (debug_state.record_state == REPLAYING) {
            if (!debug_state.replay_memory) {
                u64 file_size = platformFileSize(debug_state.record_file);
                u64 item_size = sizeof(RecordData);
                debug_state.replay_count = file_size/item_size;
                debug_state.replay_memory = platformMemoryAllocate(file_size);
                platformFileRead(debug_state.record_file, debug_state.replay_memory, file_size, 1);

                memcpy(&debug_state.replay_old_memory, &game_memory, sizeof(GameMemory));
                memcpy(&debug_state.replay_old_input, &global_frame_input, sizeof(Input));
            }

            memcpy(&global_frame_input, &debug_state.replay_memory[debug_state.replay_index].input, sizeof(Input));
            memcpy(&game_memory, &debug_state.replay_memory[debug_state.replay_index].game_memory, sizeof(GameMemory));
            debug_state.replay_index = (debug_state.replay_index + 1) % debug_state.replay_count;
            platformLog(LOG_INFO, "Replay frame: %llu/%llu", debug_state.replay_index, debug_state.replay_count);
        } else if (debug_state.record_state == IDLE && debug_state.is_record_file_open) {
            platformLog(LOG_INFO, "closing fd");
            platformFileClose(debug_state.record_file);
            debug_state.is_record_file_open = false;
            if (debug_state.replay_memory) {
                platformMemoryFree(debug_state.replay_memory);
            }
        }

        /* Call out to game modules */
        if (renderer_functions.begin_frame) {
            frame = renderer_functions.begin_frame(&renderer);
        }
        if (game_functions.update) {
            game_functions.update((1.0f/60.0f), &game_memory, &global_frame_input, frame);
        }
        if (renderer_functions.end_frame) {
            renderer_functions.end_frame(&renderer, frame);
        }

        reloadCodeModuleIfNeeded(&game_module);
        reloadCodeModuleIfNeeded(&renderer_module);

        endFrame(&frame_info);
    }

    if (renderer_functions.shutdown) {
        renderer_functions.shutdown(&renderer);
    }

    glfwDestroyWindow(renderer.window);
    glfwTerminate();

    unloadCodeModule(&game_module);
    unloadCodeModule(&renderer_module);

    sdsfree(game_path);
    sdsfree(renderer_path);
    sdsfree(dir);

    return 0;
}
