#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>

#define MAX_FRAMES_IN_FLIGHT 2

#define VK_CHECK(expr, msg)                                                     \
    do {                                                                        \
        VkResult result = expr;                                                 \
        if (result != VK_SUCCESS) {                                             \
            log_error("Vulkan: " msg " (%s)", vk_result_to_string(result));     \
        }                                                                       \
    } while(0)

typedef struct GLFWwindow GLFWwindow;

struct vkc_context {
    GLFWwindow* window;
    VkInstance instance;

    VkSurfaceKHR surface;

    VkFormat swapchain_image_format;
    VkExtent2D swapchain_extent;
    VkSwapchainKHR swapchain;
    uint32_t swapchain_image_count;
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

    uint32_t current_frame;
};

struct vkc_context *vkc_create_context(uint32_t window_width, uint32_t window_height, const char* window_title);
void vkc_free_context(struct vkc_context *context);

struct vkc_physical_device;
struct vkc_logical_device;

struct vkc_physical_device *vkc_pick_physical_device(struct vkc_context *context);
void vkc_free_physical_device(struct vkc_physical_device *device);

struct vkc_logical_device *vkc_create_logical_device(struct vkc_physical_device *device);
extern void vkc_free_logical_device(struct vkc_logical_device *device);

void vkc_create_swapchain(struct vkc_context *context, struct vkc_physical_device *physical_device, struct vkc_logical_device *logical_device);
void vkc_free_swapchain(struct vkc_context *context, struct vkc_logical_device *logical_device);

void vkc_create_swapchain_image_views(struct vkc_context *context, struct vkc_physical_device *physical_device, struct vkc_logical_device *logical_device);
void vkc_free_swapchain_image_views(struct vkc_context *context, struct vkc_logical_device *logical_device);

void vkc_create_graphics_pipeline(struct vkc_context *context, struct vkc_logical_device *logical_device);
void vkc_free_graphics_pipeline(struct vkc_context *context, struct vkc_logical_device *logical_device);

void vkc_create_render_pass(struct vkc_context *context, struct vkc_logical_device *logical_device);
void vkc_free_render_pass(struct vkc_context *context, struct vkc_logical_device *logical_device);

void vkc_create_framebuffers(struct vkc_context *context, struct vkc_logical_device *logical_device);
void vkc_free_framebuffers(struct vkc_context *context, struct vkc_logical_device *logical_device);

void vkc_create_command_pool(struct vkc_context *context, struct vkc_logical_device *logical_device, struct vkc_physical_device *physical_device);
void vkc_free_command_pool(struct vkc_context *context, struct vkc_logical_device *logical_device);

void vkc_create_command_buffers(struct vkc_context *context, struct vkc_logical_device *logical_device);

void vkc_command_buffer_rec_begin(struct vkc_context *context, struct vkc_logical_device *logical_device, uint32_t index);
void vkc_command_buffer_rec_end(struct vkc_context *context, struct vkc_logical_device *logical_device, uint32_t index);
void vkc_render_pass_start(struct vkc_context *context, struct vkc_logical_device *logical_device, uint32_t index);
void vkc_render_pass_end(struct vkc_context *context, struct vkc_logical_device *logical_device, uint32_t index);

void vkc_do_stuff(struct vkc_context *context, struct vkc_logical_device *logical_device);

void vkc_create_semaphore(struct vkc_context *context, struct vkc_logical_device *logical_device);
void vkc_free_semaphore(struct vkc_context *context, struct vkc_logical_device *logical_device);

void vkc_draw_frame(struct vkc_context *context, struct vkc_logical_device *logical_device);
