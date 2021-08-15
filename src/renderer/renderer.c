#include <shared/api.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

struct platform_function_table platform;

#define VKC_LOG_ERROR(...) \
    platform.log(LOG_ERROR, __VA_ARGS__)

#define VKC_LOG_INFO(...) \
    platform.log(LOG_INFO, __VA_ARGS__)

#include "vkc/vkc.c"

struct vkc_context *context;

static inline void setup_globals(struct renderer *r) {
    platform = r->platform;
    context = r->context;
}

void startup(struct renderer *r) {
    r->context = r->platform.allocate_memory(sizeof(struct vkc_context));
    memset(r->context, 0, sizeof(struct vkc_context));
    setup_globals(r);

    /* Extensions */
    u32 glfw_extension_count = 0;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    const char *extensions[glfw_extension_count + 1];
    extensions[0] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    memcpy(&extensions[1], glfw_extensions, sizeof(glfw_extensions[0])*glfw_extension_count);

    /* Layers */
    const char *layers[] = {
        "VK_LAYER_KHRONOS_validation",
    };

    context->instance = vkc_create_instance(extensions, ARRLEN(extensions), layers, ARRLEN(layers));

    VK_CHECK(glfwCreateWindowSurface(context->instance, r->window, NULL, &context->surface), "GLFW: failed to create window surface.");

    platform.log(LOG_INFO, "Hello form start");
}

void shutdown(struct renderer *r) {
    setup_globals(r);
    vkDestroySurfaceKHR(context->instance, context->surface, NULL);
    vkDestroyInstance(context->instance, NULL);
    platform.log(LOG_INFO, "Hello form shutdown");
}

struct render_commands *begin_frame(struct renderer *r) {
    setup_globals(r);
    r->cmds.memory_base = &r->memory[0];
    r->cmds.memory_top  = 0;
    r->cmds.memory_size = RENDERER_MEMORY_SIZE;
    return &r->cmds;
}

void end_frame(struct renderer *r, struct render_commands *cmds) {
    struct render_entry_header *header;

    header = (struct render_entry_header *) cmds->memory_base;
    switch (header->type) {
        case ENTRY_TYPE_render_entry_quad: {
            //r->platform.log(LOG_INFO, "Hello from quad\n");
            break;
        }
    };
}
