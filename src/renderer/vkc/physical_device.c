struct physical_device_info {
};

/* glfwGetPhysicalDevicePresentationSupport? */
static void populate_physical_device_info(VkPhysicalDevice device, VkSurfaceKHR surface) {
    /* Swap chain details */
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &device->capabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &device->format_count, NULL);
    if (device->format_count > 0) {
        device->formats = malloc(device->format_count * sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, context->surface, &device->format_count, device->formats);
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(device, context->surface, &device->present_mode_count, NULL);
    if (device->present_mode_count > 0) {
        device->present_modes = malloc(device->present_mode_count * sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &device->present_mode_count, device->present_modes);
    }

    /* Queue families */
    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

    VkQueueFamilyProperties queue_families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);
    for (u32 i = 0; i < queue_family_count; ++i) {
        VkQueueFamilyProperties qf = queue_families[i];

        if (qf.queueCount > 0 && qf.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            device->graphics_family = i;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
        if (qf.queueCount > 0 && present_support) {
            device->present_family = i;
        }

        if (device->graphics_family >= 0 && device->present_family >= 0) {
            break;
        }
    }
}

static bool is_physical_device_suitable(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceProperties(device, &device_properties);
    vkGetPhysicalDeviceFeatures(device, &device_features);

    VKC_LOG_INFO("Vulkan: checking device extension support");
    u32 vk_extension_count = 0;
    vkEnumerateDeviceExtensionProperties(device, NULL, &vk_extension_count, NULL);
    VkExtensionProperties vk_available_extensions[vk_extension_count];
    vkEnumerateDeviceExtensionProperties(device, NULL, &vk_extension_count, vk_available_extensions);

    bool device_extensions_supported = true;
    {
        bool found = false;
        for (u32 i = 0; i < vk_extension_count; ++i) {
            if(strcmp(vk_available_extensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                found = true;
                break;
            }
        }

        VKC_LOG_INFO("  %-38s%-20s", VK_KHR_SWAPCHAIN_EXTENSION_NAME, (found) ? "\033[0;32m[found]\033[0m" : "\033[0;31m[missing]\033[0m");

        if (!found) {
            device_extensions_supported = false;
        }
    }

    bool swap_chain_adequate = false;
    if (device_extensions_supported) {
        populate_physical_device_info(context, device);
        swap_chain_adequate = (device->formats != NULL) && (device->present_modes != NULL);
    }

    bool suitable =
        device_features.geometryShader &&
        swap_chain_adequate &&
        device_features.samplerAnisotropy &&
        device->graphics_family >= 0 &&
        device->present_family >= 0;

    VKC_LOG_INFO("%-40s%-20s",   device_properties.deviceName,
            (suitable) ?
            "\033[0;32m[ok]\033[0m" : "\033[0;31m[failed]\033[0m");
    VKC_LOG_INFO("  %-38s%-20s", "Geometry Shader",
            (device_features.geometryShader) ?
            "\033[0;32m[ok]\033[0m" : "\033[0;31m[failed]\033[0m");
    VKC_LOG_INFO("  %-38s%-20s", "Swap Chain Adequate",
            (swap_chain_adequate) ?
            "\033[0;32m[ok]\033[0m" : "\033[0;31m[failed]\033[0m");
    VKC_LOG_INFO("  %-38s%-20s", "Sampler Anisotropy",
            (device_features.samplerAnisotropy) ?
            "\033[0;32m[ok]\033[0m" : "\033[0;31m[failed]\033[0m");
    VKC_LOG_INFO("  %-38s%-20s", "Graphics family"
            ,(device->graphics_family >= 0) ?
            "\033[0;32m[ok]\033[0m" : "\033[0;31m[failed]\033[0m");
    VKC_LOG_INFO("  %-38s%-20s", "Present family",
            (device->present_family >= 0) ?
            "\033[0;32m[ok]\033[0m" : "\033[0;31m[failed]\033[0m");

    return suitable;
}

static bool vkc_pick_physical_device(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDevice *device) {
    *device = VK_NULL_HANDLE;
    VKC_LOG_INFO("Vulkan: checking suitable devices");

    u32 vk_available_devices_count = 0;
    vkEnumeratePhysicalDevices(instance, &vk_available_devices_count, NULL);
    VkPhysicalDevice vk_available_devices[vk_available_devices_count];
    vkEnumeratePhysicalDevices(instance, &vk_available_devices_count, vk_available_devices);
    for (u32 i = 0; i < device_count; ++i) {
        if(is_physical_device_suitable(vk_available_devices[i])) {
            *device = vk_available_devices[i];
            break;
        }
    }

    if (*device == VK_NULL_HANDLE) {
        VKC_LOG_ERROR("Vulkan: failed to find suitable physical device.");
        return false
    }

    return true;
}

void vkc_free_physical_device(struct vkc_physical_device *device) {
    free_physical_device_info(device);
    free(device);
}

struct vkc_logical_device *vkc_create_logical_device(struct vkc_physical_device *physical_device) {
    struct vkc_logical_device *logical_device = malloc(sizeof(struct vkc_logical_device));

    uint32_t queue_create_info_count = 0;
    VkDeviceQueueCreateInfo queue_create_infos[2]; // we're only dealing with graphics and present family
    float queue_priority = 1.0f;
    // we're only dealing with graphics and present family here, so it's a simple "if",
    // generally we check for unique queue families
    if(physical_device->graphics_family != physical_device->present_family) {
        VkDeviceQueueCreateInfo* create_info;

        create_info                   = &queue_create_infos[queue_create_info_count];
        create_info->sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        create_info->queueFamilyIndex = physical_device->graphics_family;
        create_info->queueCount       = 1;
        create_info->pQueuePriorities = &queue_priority;
        create_info->flags            = 0;
        create_info->pNext            = NULL;
        queue_create_info_count++;

        create_info                   = &queue_create_infos[queue_create_info_count];
        create_info->sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        create_info->queueFamilyIndex = physical_device->present_family;
        create_info->queueCount       = 1;
        create_info->pQueuePriorities = &queue_priority;
        create_info->flags            = 0;
        create_info->pNext            = NULL;
        queue_create_info_count++;
    } else {
        VkDeviceQueueCreateInfo* create_info;

        create_info                   = &queue_create_infos[queue_create_info_count];
        create_info->sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        create_info->queueFamilyIndex = physical_device->graphics_family;
        create_info->queueCount       = 1;
        create_info->pQueuePriorities = &queue_priority;
        create_info->flags            = 0;
        create_info->pNext            = NULL;
        queue_create_info_count++;
    }

    VkPhysicalDeviceFeatures device_features = {0};
    //device_features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo create_info = {0};
    create_info.sType                 = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos     = &queue_create_infos[0];
    create_info.queueCreateInfoCount  = queue_create_info_count;
    create_info.pEnabledFeatures      = &device_features;
    create_info.flags                 = 0;
    // Newer versions of vulkan doesn't distinguish between device and instance level
    // validation layers, so they will not be specified here.
    create_info.enabledLayerCount     = 0;

    const char* extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    create_info.enabledExtensionCount = 1;
    create_info.ppEnabledExtensionNames = &extension;

    VkResult result = vkCreateDevice(physical_device->device, &create_info, NULL, &logical_device->device);
    if(result != VK_SUCCESS) {
        log_error("Vulkan: failed to create logical device (%s).", vk_result_to_string(result));
        return NULL;
    }

    vkGetDeviceQueue(logical_device->device, physical_device->graphics_family, 0, &logical_device->graphics_queue);
    vkGetDeviceQueue(logical_device->device, physical_device->present_family,  0, &logical_device->present_queue);

    return logical_device;
}

void vkc_free_logical_device(struct vkc_logical_device *device) {
    vkDestroyDevice(device->device, NULL);
    free(device);
}

static VkSurfaceFormatKHR choose_swap_chain_surface_format(VkSurfaceFormatKHR *formats, uint32_t format_count) {
    for (uint32_t i = 0; i < format_count; ++i) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return formats[i];
        }
    }

    return formats[0];
}

static VkPresentModeKHR choose_swap_chain_present_mode(VkPresentModeKHR *modes, uint32_t mode_count) {
    for (uint32_t i = 0; i < mode_count; ++i) {
        if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return modes[i];
        }
    }

    // Guaranteed to be available
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D choose_swap_chain_extent(const struct vkc_context *context, const VkSurfaceCapabilitiesKHR *capabilities) {
    if (capabilities->currentExtent.width != UINT32_MAX) {
        return capabilities->currentExtent;
    } else {
        int width = 0, height = 0;
        glfwGetFramebufferSize(context->window, &width, &height);

        VkExtent2D actual_extent = {
            CLAMP((uint32_t)width, 	capabilities->minImageExtent.width,  capabilities->maxImageExtent.width),
            CLAMP((uint32_t)height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height),
        };

        return actual_extent;
    }
}

void vkc_create_swapchain(struct vkc_context *context, struct vkc_physical_device *physical_device, struct vkc_logical_device *logical_device) {
    VkSurfaceFormatKHR surface_format = choose_swap_chain_surface_format(physical_device->formats, physical_device->format_count);
    VkPresentModeKHR present_mode = choose_swap_chain_present_mode(physical_device->present_modes, physical_device->present_mode_count);
    VkExtent2D extent = choose_swap_chain_extent(context, &physical_device->capabilities);

    // Recommended to use one more than minimum for safety.
    uint32_t image_count = physical_device->capabilities.minImageCount + 1;
    if (physical_device->capabilities.maxImageCount > 0 && image_count > physical_device->capabilities.maxImageCount) {
        image_count = physical_device->capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = context->surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queue_indices[] = {physical_device->graphics_family, physical_device->present_family};
    if (physical_device->graphics_family != physical_device->present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = ARRLEN(queue_indices);
        create_info.pQueueFamilyIndices = queue_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = NULL;
    }

    create_info.preTransform = physical_device->capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(logical_device->device, &create_info, NULL, &context->swapchain);
    if(result != VK_SUCCESS) {
        log_error("Vulkan: failed to create swapchain (%s).", vk_result_to_string(result));
    }

    vkGetSwapchainImagesKHR(logical_device->device, context->swapchain, &image_count, NULL);
    context->swapchain_images = malloc(sizeof(VkImage) * image_count);
    context->swapchain_image_count = image_count;
    vkGetSwapchainImagesKHR(logical_device->device, context->swapchain, &image_count, context->swapchain_images);

    context->swapchain_image_format = surface_format.format;
    context->swapchain_extent = extent;
}

void vkc_free_swapchain(struct vkc_context *context, struct vkc_logical_device *logical_device) {
    free(context->swapchain_images);
    vkDestroySwapchainKHR(logical_device->device, context->swapchain, NULL);
}

void vkc_create_swapchain_image_views(struct vkc_context *context, struct vkc_physical_device *physical_device, struct vkc_logical_device *logical_device) {
    context->swapchain_image_views = malloc(sizeof(VkImageView) * context->swapchain_image_count);

    for (uint32_t i = 0; i < context->swapchain_image_count; ++i) {
        VkImageViewCreateInfo create_info = {0};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = context->swapchain_images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = context->swapchain_image_format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(logical_device->device, &create_info, NULL, &context->swapchain_image_views[i]);
        if(result != VK_SUCCESS) {
            log_error("Vulkan: failed to create swapchain image view (%s).", vk_result_to_string(result));
        }
    }
}

void vkc_free_swapchain_image_views(struct vkc_context *context, struct vkc_logical_device *logical_device) {
    for (uint32_t i = 0; i < context->swapchain_image_count; ++i) {
        vkDestroyImageView(logical_device->device, context->swapchain_image_views[i], NULL);
    }
    free(context->swapchain_image_views);
}
