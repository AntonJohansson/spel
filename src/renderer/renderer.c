#include <shared/api.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

PlatformFunctionTable platform;

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

#define MAX_FRAMES_IN_FLIGHT 2

/* TODO(anjo): Separate queue for transfer operations? */

RenderContext *context;

/* NOTE(anjo): typedef'd in api.h */
struct RenderContext {
    VkInstance instance;
    VkSurfaceKHR surface;

    struct vkc_physical_device physical_device;
    struct vkc_logical_device logical_device;

    struct vkc_swapchain swapchain;

    VkRenderPass renderpass;

    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet *descriptor_sets;

    struct vkc_pipeline pipeline;

    u32 framebuffer_count;
    VkFramebuffer *framebuffers;

    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;

    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;

    /* One for each swapchain image */
    VkBuffer *uniform_buffers;
    VkDeviceMemory *uniform_buffers_memory;

    VkCommandPool command_pool;
    VkCommandBuffer *command_buffers;

    VkSemaphore sem_image_available[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore sem_render_finished[MAX_FRAMES_IN_FLIGHT];
    VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];
    VkFence *in_flight_images;

    u32 current_frame_index;
};

struct vertex {
    f32 pos[2];
};

struct uniform_buffer_object {
    _Alignas(16) f32 pos[2];
    _Alignas(16) f32 col[3];
};

/* vertex buffer */
const struct vertex vertices[] = {
    {{-0.1f, -0.1f}},
    {{ 0.1f, -0.1f}},
    {{ 0.1f,  0.1f}},
    {{-0.1f,  0.1f}},
};

const u16 indices[] = {
    0, 1, 2, 2, 3, 0
};

static inline u32 find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_prop;
    vkGetPhysicalDeviceMemoryProperties(context->physical_device.handle, &mem_prop);
    for (u32 i = 0; i < mem_prop.memoryTypeCount; ++i) {
        if (type_filter & (1 << i) && (mem_prop.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    platform.log(LOG_INFO, "find_memory_type: failed, couldn't find memory type!");
}

static void create_buffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *memory) {
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VKC_CHECK(vkCreateBuffer(device, &buffer_info, NULL, buffer), "failed to create buffer!");

    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(device, *buffer, &mem_req);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_req.size,
        .memoryTypeIndex = find_memory_type(mem_req.memoryTypeBits, properties),
    };

    VKC_CHECK(vkAllocateMemory(device, &alloc_info, NULL, memory), "failed to allocate memory for buffer");

    vkBindBufferMemory(device, *buffer, *memory, 0);
}

static void copy_buffer(VkDevice device, VkQueue queue, VkCommandPool command_pool, VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    /* TODO(anjo): Separate command pool for temporary commands */
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = command_pool,
        .commandBufferCount = 1,
    };

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(command_buffer, &begin_info);

    VkBufferCopy copy_region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };
    vkCmdCopyBuffer(command_buffer, src, dst, 1, &copy_region);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pCommandBuffers = &command_buffer,
        .commandBufferCount = 1,
    };
    vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    /* 
     * TODO(anjo): We are performing and waiting for one copy at a time.
     * We could instead use a fence and perform multiple ones
     */
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

static void create_descriptor_set_layout(VkDevice device, VkDescriptorSetLayout *descriptor_set_layout) {
    VkDescriptorSetLayoutBinding ubo_layout_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = NULL,
    };

    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &ubo_layout_binding,
    };

    VKC_CHECK(vkCreateDescriptorSetLayout(device, &layout_info, NULL, descriptor_set_layout), "failed to create descriptor set layout");
}

static void create_uniform_buffers(VkDevice device, VkDeviceSize size, VkBuffer *buffers, VkDeviceMemory *memory, u32 count) {
    for (u32 i = 0; i < count; ++i) {
        create_buffer(device, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffers[i], &memory[i]);
    }
}

static void create_descriptor_pool(VkDevice device, u32 count, VkDescriptorPool *descriptor_pool) {
    VkDescriptorPoolSize pool_size = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = count,
    };

    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 1,
        .pPoolSizes = &pool_size,
        .maxSets = count,
    };

    VKC_CHECK(vkCreateDescriptorPool(device, &pool_info, NULL, descriptor_pool), "failed to create descriptor pool!"); 
}

static void create_descriptor_sets(VkDevice device, u32 count, VkDescriptorPool *descriptor_pool, VkDescriptorSetLayout descriptor_set_layout, VkDescriptorSet *descriptor_sets) {
    VkDescriptorSetLayout layouts[count];
    for (u32 i = 0; i < count; ++i) {
        layouts[i] = descriptor_set_layout;
    }

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = *descriptor_pool,
        .descriptorSetCount = count,
        .pSetLayouts = layouts,
    };

    VKC_CHECK(vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets), "failed to allocate descriptor sets!");
}

static void record_command_buffers() {
    {
        VkClearValue clear_color = {{{1.0f, 1.0f, 1.0f, 1.0f}}};

        VkRenderPassBeginInfo pass_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = context->renderpass,
            .framebuffer = VK_NULL_HANDLE,
            .renderArea = {
                .offset = {0, 0},
                .extent = context->swapchain.image_extent,
            },
            .clearValueCount = 1,
            .pClearValues = &clear_color,
        };

        VkCommandBufferBeginInfo cmd_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = 0,
            .pInheritanceInfo = NULL,
        };

        for (u32 i = 0; i < context->swapchain.image_count; ++i) {
            VKC_CHECK(vkBeginCommandBuffer(context->command_buffers[i], &cmd_info),
                      "failed to start recording command buffer");
            pass_info.framebuffer = context->framebuffers[i];
            vkCmdBeginRenderPass(context->command_buffers[i], &pass_info, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(context->command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline.handle);
            VkBuffer vertex_buffers[] = {context->vertex_buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(context->command_buffers[i], 0, 1, vertex_buffers, offsets);
            vkCmdBindIndexBuffer(context->command_buffers[i], context->index_buffer, 0, VK_INDEX_TYPE_UINT16);
            vkCmdBindDescriptorSets(context->command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline.layout, 0, 1, &context->descriptor_sets[i], 0, NULL);
            vkCmdDrawIndexed(context->command_buffers[i], ARRLEN(indices), 1, 0, 0, 0);

            vkCmdEndRenderPass(context->command_buffers[i]);
            VKC_CHECK(vkEndCommandBuffer(context->command_buffers[i]),
                      "failed to end recording command buffer");
        }
    }
}

static void cleanup_swapchain() {
    for (u32 i = 0; i < context->swapchain.image_count; ++i) {
        vkDestroyBuffer(context->logical_device.handle, context->uniform_buffers[i], NULL);
        vkFreeMemory(context->logical_device.handle, context->uniform_buffers_memory[i], NULL);
    }
    vkDestroyDescriptorPool(context->logical_device.handle, context->descriptor_pool, NULL);
    vkc_destroy_command_buffers(context->logical_device.handle, context->command_pool, context->command_buffers, context->swapchain.image_count);

    vkc_destroy_framebuffers(context->logical_device.handle, context->framebuffers, context->framebuffer_count);
    vkc_destroy_pipeline(context->logical_device.handle, context->pipeline);
    vkDestroyRenderPass(context->logical_device.handle, context->renderpass, NULL);
    vkc_destroy_swapchain_image_views(&context->logical_device, &context->swapchain);
    vkc_destroy_swapchain(&context->logical_device, &context->swapchain);
}

static void recreate_swapchain(Renderer *r) {
    VKC_CHECK(vkDeviceWaitIdle(context->logical_device.handle), "wait idle failed");

    cleanup_swapchain();

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

    VkVertexInputBindingDescription binding_description = {
        .binding = 0,
        .stride = sizeof(struct vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    VkVertexInputAttributeDescription attribute_descriptions[1] = {
        [0] = {
            .binding = 0,
            .location = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(struct vertex, pos),
        },
    };

    create_descriptor_pool(context->logical_device.handle, context->swapchain.image_count, &context->descriptor_pool);
    create_descriptor_sets(context->logical_device.handle, context->swapchain.image_count, &context->descriptor_pool, context->descriptor_set_layout, context->descriptor_sets);

    context->pipeline = vkc_create_pipeline(context->logical_device.handle, context->renderpass, &context->swapchain, vert_module, frag_module, binding_description, attribute_descriptions, ARRLEN(attribute_descriptions), &context->descriptor_set_layout);

    vkDestroyShaderModule(context->logical_device.handle, vert_module, NULL);
    vkDestroyShaderModule(context->logical_device.handle, frag_module, NULL);

    /* framebuffer */
    context->framebuffers = vkc_create_framebuffers(context->logical_device.handle, context->renderpass, &context->swapchain);
    context->framebuffer_count = context->swapchain.image_view_count;

    create_uniform_buffers(context->logical_device.handle, sizeof(struct uniform_buffer_object), context->uniform_buffers, context->uniform_buffers_memory, context->swapchain.image_count);
    /* Populate descriptor sets */
    {
        for (u32 i = 0; i < context->swapchain.image_count; ++i) {
            VkDescriptorBufferInfo buffer_info = {
                .buffer = context->uniform_buffers[i],
                .offset = 0,
                .range = sizeof(struct uniform_buffer_object),
            };

            VkWriteDescriptorSet descriptor_write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = context->descriptor_sets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .pBufferInfo = &buffer_info,
                .pImageInfo = NULL,
                .pTexelBufferView = NULL,
            };

            vkUpdateDescriptorSets(context->logical_device.handle, 1, &descriptor_write, 0, NULL);
        }
    }

    /* command buffer */
    context->command_buffers = vkc_create_command_buffers(context->logical_device.handle, context->command_pool, context->swapchain.image_count);
    record_command_buffers();

    for (u32 i = 0; i < context->swapchain.image_count; ++i) {
        context->in_flight_images[i] = VK_NULL_HANDLE;
    }
}

static inline void setup_globals(Renderer *r) {
    platform = r->platform;
    context = r->context;
}

void startup(Renderer *r) {
    r->context = r->platform.allocate_memory(sizeof(RenderContext));
    memset(r->context, 0, sizeof(RenderContext));
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

    VKC_CHECK(glfwCreateWindowSurface(context->instance, r->window, NULL, &context->surface), "GLFW: failed to create window surface.");

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

    VkVertexInputBindingDescription binding_description = {
        .binding = 0,
        .stride = sizeof(struct vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    VkVertexInputAttributeDescription attribute_descriptions[1] = {
        [0] = {
            .binding = 0,
            .location = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(struct vertex, pos),
        },
    };

    create_descriptor_set_layout(context->logical_device.handle, &context->descriptor_set_layout);
    create_descriptor_pool(context->logical_device.handle, context->swapchain.image_count, &context->descriptor_pool);
    context->descriptor_sets = platform.allocate_memory(sizeof(VkDescriptorSet) * context->swapchain.image_count);
    create_descriptor_sets(context->logical_device.handle, context->swapchain.image_count, &context->descriptor_pool, context->descriptor_set_layout, context->descriptor_sets);

    context->pipeline = vkc_create_pipeline(context->logical_device.handle, context->renderpass, &context->swapchain, vert_module, frag_module, binding_description, attribute_descriptions, ARRLEN(attribute_descriptions), &context->descriptor_set_layout);

    vkDestroyShaderModule(context->logical_device.handle, vert_module, NULL);
    vkDestroyShaderModule(context->logical_device.handle, frag_module, NULL);

    /* framebuffer */
    context->framebuffers = vkc_create_framebuffers(context->logical_device.handle, context->renderpass, &context->swapchain);
    context->framebuffer_count = context->swapchain.image_view_count;

    /* command buffer */
    context->command_pool = vkc_create_command_pool(context->logical_device.handle, context->physical_device.graphics_family);
    context->command_buffers = vkc_create_command_buffers(context->logical_device.handle, context->command_pool, context->swapchain.image_count);

    context->uniform_buffers = platform.allocate_memory(sizeof(VkBuffer) * context->swapchain.image_count);
    context->uniform_buffers_memory = platform.allocate_memory(sizeof(VkDeviceMemory) * context->swapchain.image_count);
    create_uniform_buffers(context->logical_device.handle, sizeof(struct uniform_buffer_object), context->uniform_buffers, context->uniform_buffers_memory, context->swapchain.image_count);
    /* Populate descriptor sets */
    {
        for (u32 i = 0; i < context->swapchain.image_count; ++i) {
            VkDescriptorBufferInfo buffer_info = {
                .buffer = context->uniform_buffers[i],
                .offset = 0,
                .range = sizeof(struct uniform_buffer_object),
            };

            VkWriteDescriptorSet descriptor_write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = context->descriptor_sets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .pBufferInfo = &buffer_info,
                .pImageInfo = NULL,
                .pTexelBufferView = NULL,
            };

            vkUpdateDescriptorSets(context->logical_device.handle, 1, &descriptor_write, 0, NULL);
        }
    }

    /* Create sync primitives */
    {
        VkSemaphoreCreateInfo sem_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        context->in_flight_images = platform.allocate_memory(sizeof(VkFence) * context->swapchain.image_count);
        memset(context->in_flight_images, 0, sizeof(VkFence) * context->swapchain.image_count);

        for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            VKC_CHECK(vkCreateSemaphore(context->logical_device.handle, &sem_info, NULL, &context->sem_image_available[i]),
                      "failed to create semaphore");

            VKC_CHECK(vkCreateSemaphore(context->logical_device.handle, &sem_info, NULL, &context->sem_render_finished[i]),
                      "failed to create semaphore");

            VKC_CHECK(vkCreateFence(context->logical_device.handle, &fence_info, NULL, &context->in_flight_fences[i]),
                      "failed to create fence");
        }
    }

    /* Copy buffer to GPU */
    {

        VkDeviceSize size = sizeof(vertices);

        VkBuffer staging_buffer;
        VkDeviceMemory staging_buffer_memory;
        create_buffer(context->logical_device.handle, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);

        void *data;
        vkMapMemory(context->logical_device.handle, staging_buffer_memory, 0, size, 0, &data);
        memcpy(data, vertices, size);
        vkUnmapMemory(context->logical_device.handle, staging_buffer_memory);

        create_buffer(context->logical_device.handle, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &context->vertex_buffer, &context->vertex_buffer_memory);
        copy_buffer(context->logical_device.handle, context->logical_device.graphics_queue, context->command_pool, staging_buffer, context->vertex_buffer, size);

        vkDestroyBuffer(context->logical_device.handle, staging_buffer, NULL);
        vkFreeMemory(context->logical_device.handle, staging_buffer_memory, NULL);
    }

    {

        VkDeviceSize size = sizeof(indices);

        VkBuffer staging_buffer;
        VkDeviceMemory staging_buffer_memory;
        create_buffer(context->logical_device.handle, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);

        void *data;
        vkMapMemory(context->logical_device.handle, staging_buffer_memory, 0, size, 0, &data);
        memcpy(data, indices, size);
        vkUnmapMemory(context->logical_device.handle, staging_buffer_memory);

        create_buffer(context->logical_device.handle, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &context->index_buffer, &context->index_buffer_memory);
        copy_buffer(context->logical_device.handle, context->logical_device.graphics_queue, context->command_pool, staging_buffer, context->index_buffer, size);

        vkDestroyBuffer(context->logical_device.handle, staging_buffer, NULL);
        vkFreeMemory(context->logical_device.handle, staging_buffer_memory, NULL);
    }

    record_command_buffers();

    platform.log(LOG_INFO, "Hello form start");
}

void shutdown(Renderer *r) {
    setup_globals(r);
    vkDeviceWaitIdle(context->logical_device.handle);

    cleanup_swapchain(r);

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(context->logical_device.handle, context->sem_render_finished[i], NULL);
        vkDestroySemaphore(context->logical_device.handle, context->sem_image_available[i], NULL);
        vkDestroyFence(context->logical_device.handle, context->in_flight_fences[i], NULL);
    }

    platform.free_memory(context->uniform_buffers);
    platform.free_memory(context->uniform_buffers_memory);

    vkDestroyDescriptorSetLayout(context->logical_device.handle, context->descriptor_set_layout, NULL);
    platform.free_memory(context->descriptor_sets);

    vkDestroyCommandPool(context->logical_device.handle, context->command_pool, NULL);

    vkFreeMemory(context->logical_device.handle, context->vertex_buffer_memory, NULL);
    vkDestroyBuffer(context->logical_device.handle, context->vertex_buffer, NULL);
    vkFreeMemory(context->logical_device.handle, context->index_buffer_memory, NULL);
    vkDestroyBuffer(context->logical_device.handle, context->index_buffer, NULL);

    vkDestroyDevice(context->logical_device.handle, NULL);
    vkDestroySurfaceKHR(context->instance, context->surface, NULL);
    vkDestroyInstance(context->instance, NULL);
    platform.log(LOG_INFO, "Hello form shutdown");
}

RenderCommands *begin_frame(Renderer *r) {
    setup_globals(r);
    r->cmds.memory_base = &r->memory[0];
    r->cmds.memory_top  = 0;
    r->cmds.memory_size = RENDERER_MEMORY_SIZE;
    return &r->cmds;
}

void end_frame(Renderer *r, RenderCommands *cmds) {
    vkWaitForFences(context->logical_device.handle, 1, &context->in_flight_fences[context->current_frame_index], VK_TRUE, UINT64_MAX);

    u32 image_index = 0;
    {
        VkResult result = vkAcquireNextImageKHR(context->logical_device.handle, context->swapchain.handle, UINT64_MAX, context->sem_image_available[context->current_frame_index], VK_NULL_HANDLE, &image_index);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreate_swapchain(r);
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            platform.log(LOG_ERROR, "Vulkan: failed to acquire next image (%s)", vk_result_to_string(result));
            return;
        }
    }

    RenderEntryHeader *header;

    header = (RenderEntryHeader *) cmds->memory_base;
    switch (header->type) {
    case ENTRY_TYPE_RenderEntryQuad: {
        RenderEntryQuad *quad = (RenderEntryQuad *) header;
        /* update uniform buffers */
        {
            struct uniform_buffer_object ubo = {0};
            v2AssignToArray(ubo.pos, quad->pos);
            colorRGBAssignToArray(ubo.col, quad->col);
          
            void *data;
            vkMapMemory(context->logical_device.handle, context->uniform_buffers_memory[image_index], 0, sizeof(ubo), 0, &data);
            memcpy(data, &ubo, sizeof(ubo));
            vkUnmapMemory(context->logical_device.handle, context->uniform_buffers_memory[image_index]);
        }

        break;
    }
    };

    if (context->in_flight_images[image_index] != VK_NULL_HANDLE) {
        vkWaitForFences(context->logical_device.handle, 1, &context->in_flight_images[image_index], VK_TRUE, UINT64_MAX);
    }
    context->in_flight_images[image_index] = context->in_flight_fences[context->current_frame_index];

    VkSemaphore wait_semaphores[] = {context->sem_image_available[context->current_frame_index]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signal_semaphores[] = {context->sem_render_finished[context->current_frame_index]};

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = wait_semaphores,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &context->command_buffers[image_index],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signal_semaphores,
    };

    vkResetFences(context->logical_device.handle, 1, &context->in_flight_fences[context->current_frame_index]);
    VKC_CHECK(vkQueueSubmit(context->logical_device.graphics_queue, 1, &submit_info, context->in_flight_fences[context->current_frame_index]),
              "failed to submit draw command buffer");

    VkSwapchainKHR swapchains[] = {context->swapchain.handle};
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signal_semaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &image_index,
        .pResults = NULL, /* Returns VkResults for each present attempt, only needed when working with multiple swapchains since we acan jst use the vkQueuePresentKHR */
    };

    {
        VkResult result = vkQueuePresentKHR(context->logical_device.present_queue, &present_info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            recreate_swapchain(r);
        } else if (result != VK_SUCCESS) {
            platform.log(LOG_ERROR, "Vulkan: failed to present queue (%s)", vk_result_to_string(result));
            return;
        }
    }

    context->current_frame_index = (context->current_frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}
