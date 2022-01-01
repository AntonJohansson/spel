VkCommandPool vkc_create_command_pool(VkDevice device, i32 graphics_family) {
    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = graphics_family,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };

    VkCommandPool pool;
    VKC_CHECK(vkCreateCommandPool(device, &pool_info, NULL, &pool),
             "failed to create command pool.");

    return pool;
}

VkCommandBuffer *vkc_create_command_buffers(VkDevice device, VkCommandPool pool, u32 count) {
    VkCommandBuffer *buffers = VKC_MEMORY_ALLOC(sizeof(VkCommandBuffer) * count);

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = count,
    };

    VKC_CHECK(vkAllocateCommandBuffers(device, &alloc_info, buffers),
             "failed to allocate command buffers.");

    return buffers;
}

void vkc_destroy_command_buffers(VkDevice device, VkCommandPool pool, VkCommandBuffer *buffers, u32 count) {
    vkFreeCommandBuffers(device, pool, count, buffers);
    VKC_MEMORY_FREE(buffers);
}
