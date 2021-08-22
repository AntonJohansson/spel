struct vkc_swapchain_info {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR present_mode;
};

struct vkc_swapchain {
    VkSwapchainKHR handle;

    VkFormat image_format;
    VkColorSpaceKHR image_color_space;
    VkExtent2D image_extent;
    u32 image_count;
    VkImage *images;
    u32 image_views_count;
    VkImageView *image_views;
};

/* TODO(anjo): a lot of this should probably be handled in the selection of a physical device */
bool vkc_get_swapchain_info(VkSurfaceKHR surface, struct vkc_physical_device *physical_device, struct vkc_swapchain_info *info) {
    VKC_LOG_INFO("Vulkan: Getting swapchain info");

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device->handle, surface, &info->capabilities);

    u32 vk_format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device->handle, surface, &vk_format_count, NULL);
    VkSurfaceFormatKHR vk_formats[vk_format_count];
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device->handle, surface, &vk_format_count, vk_formats);
    bool format_found = false;
    for (u32 i = 0; i < vk_format_count; ++i) {
        if (vk_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && vk_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            info->surface_format = vk_formats[i];
            format_found = true;
            break;
        }
    }
    log_support("surface format", format_found);
    if (!format_found) {
        return false;
    }

    u32 vk_present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device->handle, surface, &vk_present_mode_count, NULL);
    VkPresentModeKHR vk_present_modes[vk_present_mode_count];
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device->handle, surface, &vk_present_mode_count, vk_present_modes);
    bool present_mode_found = false;
    for (u32 i = 0; i < vk_present_mode_count; ++i) {
        if (vk_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            info->present_mode = vk_present_modes[i];
            present_mode_found = true;
            break;
        }
        /* TODO(anjo): fallback to VK_PRESENT_MODE_FIFO_KHR? */
    }
    log_support("present mode", present_mode_found);
    if (!present_mode_found) {
        return false;
    }
}

void vkc_create_swapchain(VkSurfaceKHR surface, struct vkc_logical_device *logical_device, struct vkc_swapchain_info *info, u32 width, u32 height, struct vkc_swapchain *swapchain) {
    swapchain->image_format = info->surface_format.format;
    swapchain->image_color_space = info->surface_format.colorSpace;
    if (info->capabilities.currentExtent.width != UINT32_MAX) {
        swapchain->image_extent = info->capabilities.currentExtent;
    } else {
        swapchain->image_extent = (VkExtent2D) {
            VKC_CLAMP(width,  info->capabilities.minImageExtent.width,  info->capabilities.maxImageExtent.width),
            VKC_CLAMP(height, info->capabilities.minImageExtent.height, info->capabilities.maxImageExtent.height),
        };
    }

    /* Recommended to use one more than minimum for safety. */
    u32 image_count = info->capabilities.minImageCount + 1;
    if (info->capabilities.maxImageCount > 0 && image_count > info->capabilities.maxImageCount) {
        image_count = info->capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = image_count,
        .imageFormat = swapchain->image_format,
        .imageColorSpace = swapchain->image_color_space,
        .imageExtent = swapchain->image_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = info->capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = info->present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };
    u32 queue_indices[] = {logical_device->graphics_family, logical_device->present_family};
    if (logical_device->graphics_family != logical_device->present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = VKC_ARRLEN(queue_indices);
        create_info.pQueueFamilyIndices = queue_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = NULL;
    }

    VK_CHECK(vkCreateSwapchainKHR(logical_device->handle, &create_info, NULL, &swapchain->handle),
        "failed to create swapchain");

    vkGetSwapchainImagesKHR(logical_device->handle, swapchain->handle, &image_count, NULL);
    swapchain->image_count = image_count;
    swapchain->images = VKC_MEMORY_ALLOC(sizeof(VkImage) * image_count);
    vkGetSwapchainImagesKHR(logical_device->handle, swapchain->handle, &image_count, swapchain->images);
}

void vkc_destroy_swapchain(struct vkc_logical_device *logical_device, struct vkc_swapchain *swapchain) {
    VKC_MEMORY_FREE(swapchain->images);
    vkDestroySwapchainKHR(logical_device->handle, swapchain->handle, NULL);
}

#if 0
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
#endif
