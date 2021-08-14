#include <vkc/vkc.h>
#include "utility.h"
#include "physical_device.h"
#include "log.h"
#include <stdlib.h>

static VkShaderModule create_shader_module(struct vkc_logical_device *logical_device, const char* code, uint64_t code_size) {
    VkShaderModuleCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code_size;
    create_info.pCode = (const uint32_t*) code;

    VkShaderModule shader_module;
    VkResult result = vkCreateShaderModule(logical_device->device, &create_info, NULL, &shader_module);
    if (result != VK_SUCCESS) {
        log_error("Vulkan: failed to create shader module (%s).", vk_result_to_string(result));
    }

    return shader_module;
}

void vkc_create_graphics_pipeline(struct vkc_context *context, struct vkc_logical_device *logical_device) {
    uint64_t vert_code_size = 0, frag_code_size = 0;
    char* vert_code = read_file_to_buffer("build/res/vert.vert.spv", &vert_code_size);
    char* frag_code = read_file_to_buffer("build/res/frag.frag.spv", &frag_code_size);

    VkShaderModule vert_module = create_shader_module(logical_device, vert_code, vert_code_size);
    VkShaderModule frag_module = create_shader_module(logical_device, frag_code, frag_code_size);
    free(vert_code);
    free(frag_code);

    VkPipelineShaderStageCreateInfo vert_create_info = {0};
    vert_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_create_info.module = vert_module;
    vert_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_create_info = {0};
    frag_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_create_info.module = frag_module;
    frag_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_create_info, frag_create_info};

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.pVertexBindingDescriptions = NULL;
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    vertex_input_info.pVertexAttributeDescriptions = NULL;

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {0};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width  = (float) context->swapchain_extent.width,
        .height = (float) context->swapchain_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {0};
    scissor.offset = (VkOffset2D) {
        .x = 0, .y = 0
    };
    scissor.extent = context->swapchain_extent;

    VkPipelineViewportStateCreateInfo viewport_state = {0};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = NULL;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
    color_blend_attachment.colorWriteMask =   VK_COLOR_COMPONENT_R_BIT
            | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending = {0};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pSetLayouts = NULL;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = NULL;

    VkResult result = vkCreatePipelineLayout(logical_device->device, &pipeline_layout_info, NULL, &context->pipeline_layout);
    if (result != VK_SUCCESS) {
        log_error("Vulkan: failed to create pipeline layout (%s).", vk_result_to_string(result));
    }

    VkGraphicsPipelineCreateInfo pipeline_info = {0};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = NULL;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = NULL;
    pipeline_info.layout = context->pipeline_layout;
    pipeline_info.renderPass = context->render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(logical_device->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &context->graphics_pipeline);
    if (result != VK_SUCCESS) {
        log_error("Vulkan: failed to create graphics pipeline (%s).", vk_result_to_string(result));
    }

    vkDestroyShaderModule(logical_device->device, vert_module, NULL);
    vkDestroyShaderModule(logical_device->device, frag_module, NULL);
}

void vkc_free_graphics_pipeline(struct vkc_context *context, struct vkc_logical_device *logical_device) {
    vkDestroyPipeline(logical_device->device, context->graphics_pipeline, NULL);
    vkDestroyPipelineLayout(logical_device->device, context->pipeline_layout, NULL);
}

void vkc_create_render_pass(struct vkc_context *context, struct vkc_logical_device *logical_device) {
    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkAttachmentDescription color_attachment = {0};
    color_attachment.format = context->swapchain_image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {0};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkRenderPassCreateInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(logical_device->device, &render_pass_info, NULL, &context->render_pass);
    if (result != VK_SUCCESS) {
        log_error("Vulkan: failed to create render pass (%s).", vk_result_to_string(result));
    }
}

void vkc_free_render_pass(struct vkc_context *context, struct vkc_logical_device *logical_device) {
    vkDestroyRenderPass(logical_device->device, context->render_pass, NULL);
}

void vkc_create_framebuffers(struct vkc_context *context, struct vkc_logical_device *logical_device) {
    context->framebuffers = malloc(sizeof(VkFramebuffer) * context->swapchain_image_count);

    for (uint32_t i = 0; i < context->swapchain_image_count; ++i) {
        VkImageView attachments[] = {
            context->swapchain_image_views[i]
        };

        VkFramebufferCreateInfo fb_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = context->render_pass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = context->swapchain_extent.width,
            .height = context->swapchain_extent.height,
            .layers = 1,
        };

        VkResult result = vkCreateFramebuffer(logical_device->device, &fb_info, NULL, &context->framebuffers[i]);
        if (result != VK_SUCCESS) {
            log_error("Vulkan: failed to create frame buffer (%s).", vk_result_to_string(result));
        }
    }
}

void vkc_free_framebuffers(struct vkc_context *context, struct vkc_logical_device *logical_device) {
    for (uint32_t i = 0; i < context->swapchain_image_count; ++i) {
        vkDestroyFramebuffer(logical_device->device, context->framebuffers[i], NULL);
    }

    free(context->framebuffers);
}

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
