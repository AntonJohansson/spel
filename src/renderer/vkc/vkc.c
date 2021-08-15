#include <vulkan/vulkan.h>
#include <string.h>
#include <stdbool.h>

#ifndef VKC_LOG_ERROR
    #error define VKC_LOG_ERROR!
#endif

#ifndef VKC_LOG_INFO
    #error define VKC_LOG_INFO!
#endif

#define MAX_FRAMES_IN_FLIGHT 2

struct vkc_context {
    VkInstance instance;
    VkSurfaceKHR surface;

    /* Physical device */

    VkPhysicalDevice device;

    u32 format_count;
    u32 present_mode_count;
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    VkPresentModeKHR *present_modes;

    i32 graphics_family;
    i32 present_family;

    /* Logical device */

    VkDevice device;
    VkQueue graphics_queue;
    VkQueue present_queue;

    /* Swap chain */

    VkFormat swapchain_image_format;
    VkExtent2D swapchain_extent;
    VkSwapchainKHR swapchain;
    u32 swapchain_image_count;
    VkImage *swapchain_images;
    VkImageView *swapchain_image_views; /* count of swapchain_image_count */

    VkRenderPass render_pass;
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
