#include <vulkan/vulkan.h>
#include <string.h>
#include <stdbool.h>

#ifndef VKC_LOG_ERROR
    #error define VKC_LOG_ERROR!
#endif

#ifndef VKC_LOG_INFO
    #error define VKC_LOG_INFO!
#endif

#ifndef VKC_ABORT
    #error define VKC_ABORT!
#endif

#ifndef VKC_MEMORY_ALLOC
    #error define VKC_MEMORY_ALLOC!
#endif

#ifndef VKC_MEMORY_FREE
    #error define VKC_MEMORY_FREE!
#endif

#define MAX_FRAMES_IN_FLIGHT 2

struct vkc_context {
    VkInstance instance;
    VkSurfaceKHR surface;

    /* Swap chain */

    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;

    VkFramebuffer *framebuffers; /* count of swapchain_image_count */

    VkCommandPool command_pool;
    VkCommandBuffer *command_buffers; /* count of swapchain_image_count */

    VkSemaphore sem_image_available[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore sem_render_finished[MAX_FRAMES_IN_FLIGHT];

    VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];
    VkFence in_flight_images[MAX_FRAMES_IN_FLIGHT];

    u32 current_frame;
};

#include "vkc_util.c"
#include "vkc_instance.c"
#include "vkc_physical_device.c"
#include "vkc_logical_device.c"
#include "vkc_swapchain.c"
#include "vkc_renderpass.c"
#include "vkc_graphics_pipeline.c"
#include "vkc_framebuffer.c"
