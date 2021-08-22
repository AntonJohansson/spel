#include <vkc/vkc.h>
#include "utility.h"
#include "physical_device.h"
#include "log.h"
#include <stdlib.h>

void vkc_create_command_pool(struct vkc_context *context, struct vkc_logical_device *logical_device, struct vkc_physical_device *physical_device) {
    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = physical_device->graphics_family,
        .flags = 0,
    };

    VkResult result = vkCreateCommandPool(logical_device->device, &pool_info, NULL, &context->command_pool);
    if (result != VK_SUCCESS) {
        log_error("Vulkan: failed to create command pool (%s).", vk_result_to_string(result));
    }
}

void vkc_free_command_pool(struct vkc_context *context, struct vkc_logical_device *logical_device) {
    vkDestroyCommandPool(logical_device->device, context->command_pool, NULL);
}

void vkc_create_command_buffers(struct vkc_context *context, struct vkc_logical_device *logical_device) {
    context->command_buffers = malloc(sizeof(VkCommandBuffer) * context->swapchain_image_count);

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = context->command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = context->swapchain_image_count
    };

    VkResult result = vkAllocateCommandBuffers(logical_device->device, &alloc_info, context->command_buffers);
    if (result != VK_SUCCESS) {
        log_error("Vulkan: failed to allocate command buffers (%s).", vk_result_to_string(result));
    }
}

void vkc_command_buffer_rec_begin(struct vkc_context *context, struct vkc_logical_device *logical_device, uint32_t index) {
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = NULL,
    };

    VkResult result = vkBeginCommandBuffer(context->command_buffers[index], &begin_info);
    if (result != VK_SUCCESS) {
        log_error("Vulkan: failed to start recording command buffer (%s).", vk_result_to_string(result));
    }
}

void vkc_command_buffer_rec_end(struct vkc_context *context, struct vkc_logical_device *logical_device, uint32_t index) {
    VkResult result = vkEndCommandBuffer(context->command_buffers[index]);
    if (result != VK_SUCCESS) {
        log_error("Vulkan: failed to end recording command buffer (%s).", vk_result_to_string(result));
    }
}

void vkc_render_pass_begin(struct vkc_context *context, struct vkc_logical_device *logical_device, uint32_t index) {
    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    VkRenderPassBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = context->render_pass,
        .framebuffer = context->framebuffers[index],
        .renderArea = {
            .offset = {0, 0},
            .extent = context->swapchain_extent,
        },
        .clearValueCount = 1,
        .pClearValues = &clear_color,
    };

    vkCmdBeginRenderPass(context->command_buffers[index], &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void vkc_render_pass_end(struct vkc_context *context, struct vkc_logical_device *logical_device, uint32_t index) {
    vkCmdEndRenderPass(context->command_buffers[index]);
}

void vkc_do_stuff(struct vkc_context *context, struct vkc_logical_device *logical_device) {
    for (uint32_t i = 0; i < context->swapchain_image_count; ++i) {
        vkc_command_buffer_rec_begin(context, logical_device, i);
        vkc_render_pass_begin(context, logical_device, i);


        vkCmdBindPipeline(context->command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, context->graphics_pipeline);
        vkCmdDraw(context->command_buffers[i], 3, 1, 0, 0);

        vkc_render_pass_end(context, logical_device, i);
        vkc_command_buffer_rec_end(context, logical_device, i);
    }
}

void vkc_create_semaphore(struct vkc_context *context, struct vkc_logical_device *logical_device) {
    VkSemaphoreCreateInfo sem_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VK_CHECK(vkCreateSemaphore(logical_device->device, &sem_info, NULL, &context->sem_image_available[i]),
                "failed to create semaphore");

        VK_CHECK(vkCreateSemaphore(logical_device->device, &sem_info, NULL, &context->sem_render_finished[i]),
                "failed to create semaphore");

        VK_CHECK(vkCreateFence(logical_device->device, &fence_info, NULL, &context->in_flight_fences[i]),
                "failed to create fence");
    }
}

void vkc_free_semaphore(struct vkc_context *context, struct vkc_logical_device *logical_device) {
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(logical_device->device, context->sem_render_finished[i], NULL);
        vkDestroySemaphore(logical_device->device, context->sem_image_available[i], NULL);
        vkDestroyFence(logical_device->device, context->in_flight_fences[i], NULL);
    }
}

void vkc_draw_frame(struct vkc_context *context, struct vkc_logical_device *logical_device) {
    vkWaitForFences(logical_device->device, 1, &context->in_flight_fences[context->current_frame], VK_TRUE, UINT64_MAX);

    uint32_t image_index;
    vkAcquireNextImageKHR(logical_device->device, context->swapchain, UINT64_MAX, context->sem_image_available[context->current_frame], VK_NULL_HANDLE, &image_index);
    if (context->in_flight_images[image_index] != VK_NULL_HANDLE) {
        vkWaitForFences(logical_device->device, 1, &context->in_flight_images[context->current_frame], VK_TRUE, UINT64_MAX);
    }
    context->in_flight_images[image_index] = context->in_flight_fences[context->current_frame];

    VkSemaphore wait_semaphores[] = {context->sem_image_available[context->current_frame]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signal_semaphores[] = {context->sem_render_finished[context->current_frame]};
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pWaitSemaphores = wait_semaphores,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &context->command_buffers[image_index],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signal_semaphores,
    };

    vkResetFences(logical_device->device, 1, &context->in_flight_fences[context->current_frame]);
    VK_CHECK(vkQueueSubmit(logical_device->graphics_queue, 1, &submit_info, context->in_flight_fences[context->current_frame]),
            "failed to submit draw command buffer");

    VkSwapchainKHR swapchains[] = {context->swapchain};
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signal_semaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &image_index,
        .pResults = NULL, /* Returns VkResults for each present attempt, only needed when working with multiple swapchains since we acan jst use the vkQueuePresentKHR */
    };

    VK_CHECK(vkQueuePresentKHR(logical_device->present_queue, &present_info),
            "failed to present to queue");

    context->current_frame = (context->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

    vkDeviceWaitIdle(logical_device->device);
}
