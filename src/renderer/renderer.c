#include <shared/api.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

struct platform_function_table platform;

#define VKC_LOG_ERROR(...) \
    platform.log(LOG_ERROR, __VA_ARGS__)

#define VKC_LOG_INFO(...) \
    platform.log(LOG_INFO, __VA_ARGS__)

#define VKC_ABORT() \
    platform.abort()

#define VKC_MEMORY_ALLOC(size) \
    platform.allocate_memory(size)

#define VKC_MEMORY_FREE(size) /* Empty */

#include "vkc/vkc.c"

struct render_context *context;
struct render_context {
    VkInstance instance;
    VkSurfaceKHR surface;

    struct vkc_physical_device physical_device;
    struct vkc_logical_device logical_device;

    struct vkc_swapchain swapchain;

    VkRenderPass renderpass;

    struct vkc_pipeline pipeline;

    u32 framebuffer_count;
    VkFramebuffer *framebuffers;
};

static inline void setup_globals(struct renderer *r) {
    platform = r->platform;
    context = r->context;
}

void startup(struct renderer *r) {
    r->context = r->platform.allocate_memory(sizeof(struct render_context));
    memset(r->context, 0, sizeof(struct render_context));
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

    vkc_pick_physical_device(context->instance, context->surface, &context->physical_device);
    vkc_create_logical_device(&context->physical_device, &context->logical_device);

    struct vkc_swapchain_info swapchain_info = {0};
    vkc_get_swapchain_info(context->surface, &context->physical_device, &swapchain_info);
    int width = 0, height = 0;
    glfwGetFramebufferSize(r->window, &width, &height);
    vkc_create_swapchain(context->surface, &context->logical_device, &swapchain_info, width, height, &context->swapchain);
    vkc_create_swapchain_image_views(&context->logical_device, &context->swapchain);
    context->renderpass = vkc_create_renderpass(context->logical_device.handle, &context->swapchain);

    u8 *vert_code = NULL;
    u64 vert_code_size = 0;
    platform.read_file_to_buffer("res/vert.vert.spv", &vert_code, &vert_code_size);
    u8 *frag_code = NULL;
    u64 frag_code_size = 0;
    platform.read_file_to_buffer("res/frag.frag.spv", &frag_code, &frag_code_size);
    VkShaderModule vert_module = vkc_create_shader_module(context->logical_device.handle, vert_code, vert_code_size);
    VkShaderModule frag_module = vkc_create_shader_module(context->logical_device.handle, frag_code, frag_code_size);
    platform.free_memory(vert_code);
    platform.free_memory(frag_code);

    context->pipeline = vkc_create_pipeline(context->logical_device.handle, context->renderpass, &context->swapchain, vert_module, frag_module);

    vkDestroyShaderModule(context->logical_device.handle, vert_module, NULL);
    vkDestroyShaderModule(context->logical_device.handle, frag_module, NULL);

    context->framebuffers = vkc_create_framebuffers(context->logical_device.handle, context->renderpass, &context->swapchain);
    context->framebuffer_count = context->swapchain.image_view_count;

    platform.log(LOG_INFO, "Hello form start");
}

void shutdown(struct renderer *r) {
    setup_globals(r);
    vkc_destroy_framebuffers(context->logical_device.handle, context->framebuffers, context->framebuffer_count);
    vkc_destroy_pipeline(context->logical_device.handle, context->pipeline);
    vkDestroyRenderPass(context->logical_device.handle, context->renderpass, NULL);
    vkc_destroy_swapchain_image_views(&context->logical_device, &context->swapchain);
    vkc_destroy_swapchain(&context->logical_device, &context->swapchain);
    vkDestroyDevice(context->logical_device.handle, NULL);
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
