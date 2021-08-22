struct vkc_logical_device {
    VkDevice handle;
    VkQueue graphics_queue;
    VkQueue present_queue;

    u32 graphics_family;
    u32 present_family;
};

void vkc_create_logical_device(struct vkc_physical_device *physical_device, struct vkc_logical_device *logical_device) {
    /*
     * we're only dealing with graphics and present family here, so it's a simple "if",
     * generally we check for unique queue families
     */
    u32 queue_create_info_count = 0;
    VkDeviceQueueCreateInfo queue_create_infos[2];
    f32 queue_priority = 1.0f;
    if (physical_device->graphics_family != physical_device->present_family) {
        VkDeviceQueueCreateInfo* create_info;

        create_info = &queue_create_infos[queue_create_info_count];
        create_info->sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        create_info->queueFamilyIndex = physical_device->graphics_family;
        create_info->queueCount       = 1;
        create_info->pQueuePriorities = &queue_priority;
        create_info->flags            = 0;
        create_info->pNext            = NULL;
        queue_create_info_count++;

        create_info = &queue_create_infos[queue_create_info_count];
        create_info->sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        create_info->queueFamilyIndex = physical_device->present_family;
        create_info->queueCount       = 1;
        create_info->pQueuePriorities = &queue_priority;
        create_info->flags            = 0;
        create_info->pNext            = NULL;
        queue_create_info_count++;
    } else {
        VkDeviceQueueCreateInfo* create_info;

        create_info = &queue_create_infos[queue_create_info_count];
        create_info->sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        create_info->queueFamilyIndex = physical_device->graphics_family;
        create_info->queueCount       = 1;
        create_info->pQueuePriorities = &queue_priority;
        create_info->flags            = 0;
        create_info->pNext            = NULL;
        queue_create_info_count++;
    }

    /* TODO(anjo): why not use device_features of physical_device? */
    VkPhysicalDeviceFeatures device_features = {0};
    /* device_features.samplerAnisotropy = VK_TRUE; */

    VkDeviceCreateInfo create_info = {
        .sType                 = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos     = &queue_create_infos[0],
        .queueCreateInfoCount  = queue_create_info_count,
        .pEnabledFeatures      = &device_features,
        .flags                 = 0,
        /*
         * Newer versions of vulkan doesn't distinguish between device and instance level
         * validation layers, so they will not be specified here.
         */
        .enabledLayerCount     = 0,
    };

    /* TODO(anjo): Isn't this just a copy of the stuff from the physical_device? */
    const char* extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    create_info.enabledExtensionCount = 1;
    create_info.ppEnabledExtensionNames = &extension;

    VK_CHECK(vkCreateDevice(physical_device->handle, &create_info, NULL, &logical_device->handle),
        "failed to create logical device");

    vkGetDeviceQueue(logical_device->handle, physical_device->graphics_family, 0, &logical_device->graphics_queue);
    vkGetDeviceQueue(logical_device->handle, physical_device->present_family,  0, &logical_device->present_queue);
    logical_device->graphics_family = physical_device->graphics_family;
    logical_device->present_family = physical_device->present_family;
}
