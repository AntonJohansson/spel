#include "platform/platform.h"
#include "third_party/sds.h"

typedef void startup_func(void);
typedef void update_func(void);
typedef void shutdown_func(void);

struct code_module {
    const char *path;
    void *handle;
    startup_func  *startup;
    update_func   *update;
    shutdown_func *shutdown;
};

static void load_code_module(struct code_module *module) {
    module->handle = platform_dlopen(module->path);
    platform_dlsym((void **)&module->startup,  module->handle, "startup");
    platform_dlsym((void **)&module->update,   module->handle, "update");
    platform_dlsym((void **)&module->shutdown, module->handle, "shutdown");
}

static void unload_code_module(struct code_module *module) {
    module->shutdown();
    platform_dlclose(module->handle);
}

int main(int argc, char **argv) {
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

    struct code_module game_module = {.path = game_path};
    load_code_module(&game_module);

    struct code_module renderer_module = {.path = renderer_path};
    load_code_module(&renderer_module);

    renderer_module.startup();
    renderer_module.update();
    renderer_module.shutdown();

    unload_code_module(&game_module);
    unload_code_module(&renderer_module);

    sdsfree(game_path);
    sdsfree(renderer_path);
    sdsfree(dir);

    return 0;
}
