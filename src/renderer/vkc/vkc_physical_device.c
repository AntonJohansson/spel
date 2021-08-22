struct vkc_physical_device {
    VkPhysicalDevice handle;

    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;

    i32 graphics_family;
    i32 present_family;
};

static void log_support(const char *msg, bool ok) {
    VKC_LOG_INFO("  %-38s%-20s", msg , ok ?  "\033[0;32m[ok]\033[0m" : "\033[0;31m[failed]\033[0m");
}

static bool is_physical_device_suitable(VkSurfaceKHR surface, struct vkc_physical_device *physical_device) {
    vkGetPhysicalDeviceProperties(physical_device->handle, &physical_device->device_properties);
    VKC_LOG_INFO("Checking device: %s", physical_device->device_properties.deviceName);

    vkGetPhysicalDeviceFeatures(physical_device->handle, &physical_device->device_features);
    log_support("Geometry shader", physical_device->device_features.geometryShader);
    if (!physical_device->device_features.geometryShader) {
        return false;
    }
    log_support("Sampler Anisotropy", physical_device->device_features.samplerAnisotropy);
    if (!physical_device->device_features.samplerAnisotropy) {
        return false;
    }

    u32 vk_extension_count = 0;
    vkEnumerateDeviceExtensionProperties(physical_device->handle, NULL, &vk_extension_count, NULL);
    VkExtensionProperties vk_available_extensions[vk_extension_count];
    vkEnumerateDeviceExtensionProperties(physical_device->handle, NULL, &vk_extension_count, vk_available_extensions);
    bool found = false;
    for (u32 i = 0; i < vk_extension_count; ++i) {
        if(strcmp(vk_available_extensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            found = true;
            break;
        }
    }
    log_support(VK_KHR_SWAPCHAIN_EXTENSION_NAME, found);
    if (!found) {
        return false;
    }

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device->handle, &queue_family_count, NULL);
    VkQueueFamilyProperties queue_families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device->handle, &queue_family_count, queue_families);
    physical_device->graphics_family = -1;
    physical_device->present_family = -1;
    for (u32 i = 0; i < queue_family_count; ++i) {
        VkQueueFamilyProperties qf = queue_families[i];

        if (qf.queueCount > 0 && qf.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            physical_device->graphics_family = i;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device->handle, i, surface, &present_support);
        if (qf.queueCount > 0 && present_support) {
            physical_device->present_family = i;
        }

        if (physical_device->graphics_family >= 0 && physical_device->present_family >= 0) {
            break;
        }
    }
    log_support("Graphics family", physical_device->graphics_family >= 0);
    log_support("Present family", physical_device->present_family >= 0);
    if (physical_device->graphics_family < 0 || physical_device->present_family < 0) {
        return false;
    }

    return true;
}

static bool vkc_pick_physical_device(VkInstance instance, VkSurfaceKHR surface, struct vkc_physical_device *physical_device) {
    VKC_LOG_INFO("Vulkan: checking suitable devices");

    u32 vk_available_devices_count = 0;
    vkEnumeratePhysicalDevices(instance, &vk_available_devices_count, NULL);
    VkPhysicalDevice vk_available_devices[vk_available_devices_count];
    vkEnumeratePhysicalDevices(instance, &vk_available_devices_count, vk_available_devices);
    for (u32 i = 0; i < vk_available_devices_count; ++i) {
        physical_device->handle = vk_available_devices[i];
        if(is_physical_device_suitable(surface, physical_device)) {
            return true;
        }
    }

    VKC_LOG_ERROR("Vulkan: failed to find suitable physical device.");

    return false;
}
