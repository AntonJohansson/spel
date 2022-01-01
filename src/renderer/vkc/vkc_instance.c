static VkInstance vkc_create_instance(const char **extensions, u32 extension_count, const char **layers, u32 layer_count) {
    /* Extensions */
    VKC_LOG_INFO("Vulkan: checking extension support");
    /* Get available extensions */
    u32 vk_extension_count = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &vk_extension_count, NULL);
    VkExtensionProperties vk_available_extensions[vk_extension_count];
    vkEnumerateInstanceExtensionProperties(NULL, &vk_extension_count, vk_available_extensions);

    u32 found_extension_count = 0;
    const char* found_extensions[vk_extension_count];
    for (u32 i = 0; i < extension_count; ++i) {
        bool found = false;
        for (u32 j = 0; j < vk_extension_count; ++j) {
            if (strcmp(vk_available_extensions[j].extensionName, extensions[i]) == 0) {
                found = true;
                break;
            }
        }
        VKC_LOG_INFO("  %-38s%-20s", extensions[i], (found) ? "\033[0;32m[found]\033[0m" : "\033[0;31m[missing]\033[0m");
        found_extensions[found_extension_count++] = extensions[i];
    }

    /* Layers */
    VKC_LOG_INFO("Vulkan: checking layer support");
    /* Get available layers */
    u32 vk_layer_count = 0;
    vkEnumerateInstanceLayerProperties(&vk_layer_count, NULL);
    VkLayerProperties vk_available_layers[vk_layer_count];
    vkEnumerateInstanceLayerProperties(&vk_layer_count, vk_available_layers);

    u32 found_layer_count = 0;
    const char* found_layers[vk_layer_count];
    for (u32 i = 0; i < layer_count; ++i) {
        bool found = false;
        for (u32 j = 0; j < vk_layer_count; ++j) {
            if (strcmp(vk_available_layers[j].layerName, layers[i]) == 0) {
                found = true;
                break;
            }
        }
        VKC_LOG_INFO("  %-38s%-20s", layers[i], (found) ? "\033[0;32m[found]\033[0m" : "\033[0;31m[missing]\033[0m");
        found_layers[found_layer_count++] = layers[i];
    }

    VkApplicationInfo app_info = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = NULL,
        .pApplicationName   = "window",
        .applicationVersion = VK_MAKE_VERSION(0,0,1),
        .pEngineName        = "vkc",
        .engineVersion      = VK_MAKE_VERSION(0,0,1),
        .apiVersion         = VK_API_VERSION_1_1,
    };

    VkInstanceCreateInfo create_info = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = NULL,
        .flags                   = 0,
        .pApplicationInfo        = &app_info,
        .enabledExtensionCount   = found_extension_count,
        .ppEnabledExtensionNames = found_extensions,
        .enabledLayerCount       = found_layer_count,
        .ppEnabledLayerNames     = found_layers,
    };

    VkInstance instance;
    VKC_CHECK(vkCreateInstance(&create_info, NULL, &instance), "failed to create instance");

    return instance;
}
