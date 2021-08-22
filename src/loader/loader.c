#include "platform/platform.h"
#include "third_party/sds.h"
#include <shared/types.h>
#include <shared/api.h>
#include <shared/input.h>

#include <GLFW/glfw3.h>

/* libc */
#include <stdbool.h>
#include <string.h>

/* posix */
#include <unistd.h>

/*
 * Frame input
 */

static struct frame_input global_frame_input = {0};

static enum input_type global_key_map[GLFW_KEY_LAST] = {
    [GLFW_KEY_J] = INPUT_MOVE_LEFT,
    [GLFW_KEY_K] = INPUT_MOVE_RIGHT,
    [GLFW_KEY_F] = INPUT_JUMP,
};

static void clear_input(struct frame_input *input) {
    memset(input->active, 0, sizeof(input->active));
}

#include "glfw_input.c"

/*
 * Code modules
 */

struct code_module {
    const char *path;
    void *handle;

    u64 last_modify_time;

    u8 function_count;
    void **functions;
    char **function_names;
};

static inline void load_code_module(struct code_module *module) {
    module->handle = platform_dlopen(module->path);
    if (!module->handle) {
        platform_log(LOG_ERROR, "Failed to load module %s!", module->path);
        return;
    }

    for (u8 i = 0; i < module->function_count; ++i) {
        platform_dlsym(&module->functions[i], module->handle, module->function_names[i]);
    }
}

static inline void unload_code_module(struct code_module *module) {
    platform_dlclose(module->handle);
}

static inline void reload_module_if_needed(struct code_module *module) {
    u64 new_modify_time = platform_last_file_modify(module->path);
    if (new_modify_time > module->last_modify_time) {
        sleep(1);
        platform_log(LOG_INFO, "Reloading module %s", module->path);
        module->last_modify_time = new_modify_time;
        unload_code_module(module);
        load_code_module(module);
    }
}

/*
 *
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

    sds game_path = sdsnew(dir);
    game_path = sdscat(game_path, "/libgame");

    sds renderer_path = sdsnew(dir);
    renderer_path = sdscat(renderer_path, "/librenderer");

    struct game_function_table game_functions = {0};
    struct code_module game_module = {
        .path = game_path,
        .last_modify_time = platform_last_file_modify(game_path),
        .function_count = ARRLEN(game_function_names),
        .functions = (void **) &game_functions,
        .function_names = game_function_names,
    };
    load_code_module(&game_module);

    struct renderer_function_table renderer_functions = {0};
    struct code_module renderer_module = {
        .path = renderer_path,
        .last_modify_time = platform_last_file_modify(renderer_path),
        .function_count = ARRLEN(renderer_function_names),
        .functions = (void **) &renderer_functions,
        .function_names = renderer_function_names,
    };
    load_code_module(&renderer_module);

    struct platform_function_table platform_functions = {
        .log = platform_log,
        .allocate_memory = platform_allocate_memory,
        .free_memory = platform_free_memory,
        .abort = platform_abort,
    };

    struct game_memory memory = {
        .platform = platform_functions,
    };
    struct render_commands *frame = NULL;

    struct renderer renderer = {
        .platform = platform_functions,
    };

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, false);
    renderer.window = glfwCreateWindow(800, 600, "spel", 0, 0);
    glfwMakeContextCurrent(renderer.window);

    set_glfw_input_callbacks(renderer.window);

    if (renderer_functions.startup) {
        renderer_functions.startup(&renderer);
    }

    while (!glfwWindowShouldClose(renderer.window)) {
        clear_input(&global_frame_input);
        glfwPollEvents();

        if (renderer_functions.begin_frame) {
            frame = renderer_functions.begin_frame(&renderer);
        }
        if (game_functions.update) {
            game_functions.update(&memory, &global_frame_input, frame);
        }
        if (renderer_functions.end_frame) {
            renderer_functions.end_frame(&renderer, frame);
        }
        glfwSwapBuffers(renderer.window);

        reload_module_if_needed(&game_module);
        reload_module_if_needed(&renderer_module);
    }

    if (renderer_functions.shutdown) {
        renderer_functions.shutdown(&renderer);
    }

    glfwDestroyWindow(renderer.window);
    glfwTerminate();

    unload_code_module(&game_module);
    unload_code_module(&renderer_module);

    sdsfree(game_path);
    sdsfree(renderer_path);
    sdsfree(dir);

    return 0;
}
