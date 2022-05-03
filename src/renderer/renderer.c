#include <shared/api.h>
PlatformFunctionTable platform;


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdbool.h>

struct vkc_physical_device {
    VkPhysicalDevice handle;

    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;

    i32 graphics_family;
    i32 present_family;
};

struct vkc_logical_device {
    VkDevice handle;
    VkQueue graphics_queue;
    VkQueue present_queue;

    u32 graphics_family;
    u32 present_family;
};

struct vkc_pipeline {
    VkPipeline handle;
    VkPipelineLayout layout;
};

typedef struct SwapchainInfo {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR present_mode;
} SwapchainInfo;

typedef struct Swapchain {
    VkSwapchainKHR handle;

    VkFormat image_format;
    VkColorSpaceKHR image_color_space;
    VkExtent2D image_extent;
    u32 image_count;
    VkImage *images;
    u32 image_view_count;
    VkImageView *image_views;
} Swapchain;

#define MAX_FRAMES_IN_FLIGHT 2

/* TODO(anjo): Separate queue for transfer operations? */
/* NOTE(anjo): typedef'd in api.h */
struct RenderContext {
    VkInstance instance;
    VkSurfaceKHR surface;

    struct vkc_physical_device physical_device;
    struct vkc_logical_device logical_device;

    Swapchain swapchain;

    VkRenderPass renderpass;

    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet *descriptor_sets;

    struct vkc_pipeline color_pipeline;
    struct vkc_pipeline texture_pipeline;
    struct vkc_pipeline atlas_pipeline;

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

    VkShaderModule color_vert_module;
    VkShaderModule color_frag_module;

    VkShaderModule texture_vert_module;
    VkShaderModule texture_frag_module;

    VkShaderModule atlas_vert_module;
    VkShaderModule atlas_frag_module;

    VkImage texture_image;
    VkDeviceMemory texture_image_memory;
    VkImageView texture_image_view;
    bool has_texture;

    VkSampler texture_sampler;
};

/*
 * Globals
 */

RenderContext *context;

typedef struct Vertex {
    f32 pos[2];
    f32 texcoord[2];
} Vertex;

struct uniform_buffer_object {
    _Alignas(16) f32 pos[2];
    _Alignas(16) f32 col[3];
};

typedef struct ShapePushConstants {
    f32 pos[2];
    f32 scale[2];
    f32 col[3];
} ShapePushConstants;

typedef struct AtlasPushConstants {
    f32 pos[2];
    f32 scale[2];
    f32 offset[2];
    f32 size[2];
    f32 col[3];
} AtlasPushConstants;

/* vertex buffer */

const Vertex vertices[] = {
    {{-0.5f, -0.5f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f}, {0.0f, 1.0f}},
};

const u16 indices[] = {
    0, 1, 2, 2, 3, 0
};

/*
 * Vulkan abstraction
 */

#define VKC_CHECK(expr, msg)                                                              \
    do {                                                                                  \
        VkResult result = expr;                                                           \
        if (result != VK_SUCCESS) {                                                       \
            platform.log(LOG_ERROR, "Vulkan: " msg " (%s)", vk_result_to_string(result)); \
            platform.abort();                                                             \
        }                                                                                 \
    } while(0)

static const char* vk_result_to_string(VkResult result) {
    switch(result) {
    case VK_SUCCESS:
        return "VK_SUCCESS";
        break;
    case VK_NOT_READY:
        return "VK_NOT_READY";
        break;
    case VK_TIMEOUT:
        return "VK_TIMEOUT";
        break;
    case VK_EVENT_SET:
        return "VK_EVENT_SET";
        break;
    case VK_EVENT_RESET:
        return "VK_EVENT_RESET";
        break;
    case VK_INCOMPLETE:
        return "VK_INCOMPLETE";
        break;
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
        break;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        break;
    case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
        break;
    case VK_ERROR_DEVICE_LOST:
        return "VK_ERROR_DEVICE_LOST";
        break;
    case VK_ERROR_MEMORY_MAP_FAILED:
        return "VK_ERROR_MEMORY_MAP_FAILED";
        break;
    case VK_ERROR_LAYER_NOT_PRESENT:
        return "VK_ERROR_LAYER_NOT_PRESENT";
        break;
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
        break;
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
        break;
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
        break;
    case VK_ERROR_TOO_MANY_OBJECTS:
        return "VK_ERROR_TOO_MANY_OBJECTS";
        break;
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        break;
    case VK_ERROR_FRAGMENTED_POOL:
        return "VK_ERROR_FRAGMENTED_POOL";
        break;
    case VK_ERROR_OUT_OF_POOL_MEMORY:
        return "VK_ERROR_OUT_OF_POOL_MEMORY";
        break;
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        break;
    case VK_ERROR_SURFACE_LOST_KHR:
        return "VK_ERROR_SURFACE_LOST_KHR";
        break;
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        break;
    case VK_SUBOPTIMAL_KHR:
        return "VK_SUBOPTIMAL_KHR";
        break;
    case VK_ERROR_OUT_OF_DATE_KHR:
        return "VK_ERROR_OUT_OF_DATE_KHR";
        break;
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        break;
    case VK_ERROR_VALIDATION_FAILED_EXT:
        return "VK_ERROR_VALIDATION_FAILED_EXT";
        break;
    case VK_ERROR_INVALID_SHADER_NV:
        return "VK_ERROR_INVALID_SHADER_NV";
        break;
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
        return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        break;
    case VK_ERROR_FRAGMENTATION_EXT:
        return "VK_ERROR_FRAGMENTATION_EXT";
        break;
    case VK_ERROR_NOT_PERMITTED_EXT:
        return "VK_ERROR_NOT_PERMITTED_EXT";
        break;
    case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
        return "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT";
        break;
        //case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
        //	return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT"; break;
        //case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:
        //	return "VK_ERROR_OUT_OF_POOL_MEMORY_KHR"; break;
        //case VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR:
        //	return "VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR"; break;
    case VK_RESULT_MAX_ENUM:
        return "VK_RESULT_MAX_ENUM";
        break;
    default:
        return "Unknown VkResult";
    }
}

/* Instance */

static VkInstance vkc_create_instance(const char **extensions, u32 extension_count, const char **layers, u32 layer_count) {
    /* Extensions */
    platform.log(LOG_INFO, "Vulkan: checking extension support");
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
        platform.log(LOG_INFO, "  %-38s%-20s", extensions[i], (found) ? "\033[0;32m[found]\033[0m" : "\033[0;31m[missing]\033[0m");
        found_extensions[found_extension_count++] = extensions[i];
    }

    /* Layers */
    platform.log(LOG_INFO, "Vulkan: checking layer support");
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
        platform.log(LOG_INFO, "  %-38s%-20s", layers[i], (found) ? "\033[0;32m[found]\033[0m" : "\033[0;31m[missing]\033[0m");
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

/* Physical Device */

static void log_support(const char *msg, bool ok) {
    platform.log(LOG_INFO, "  %-38s%-20s", msg , ok ?  "\033[0;32m[ok]\033[0m" : "\033[0;31m[failed]\033[0m");
}

static bool is_physical_device_suitable(VkSurfaceKHR surface, struct vkc_physical_device *physical_device) {
    vkGetPhysicalDeviceProperties(physical_device->handle, &physical_device->device_properties);
    platform.log(LOG_INFO, "Checking device: %s", physical_device->device_properties.deviceName);

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
    platform.log(LOG_INFO, "Vulkan: checking suitable devices");

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

    platform.log(LOG_ERROR, "Vulkan: failed to find suitable physical device.");

    return false;
}

/* Logical Device */

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

    VkPhysicalDeviceFeatures device_features = {
        .samplerAnisotropy = VK_TRUE,
    };

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

    VKC_CHECK(vkCreateDevice(physical_device->handle, &create_info, NULL, &logical_device->handle),
        "failed to create logical device");

    vkGetDeviceQueue(logical_device->handle, physical_device->graphics_family, 0, &logical_device->graphics_queue);
    vkGetDeviceQueue(logical_device->handle, physical_device->present_family,  0, &logical_device->present_queue);
    logical_device->graphics_family = physical_device->graphics_family;
    logical_device->present_family = physical_device->present_family;
}

/* Command buffers and pools */

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
    VkCommandBuffer *buffers = platform.allocate_memory(sizeof(VkCommandBuffer) * count);

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
    platform.free_memory(buffers);
}

static VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool command_pool) {
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

    return command_buffer;
}

static void endSingleTimeCommands(VkDevice device, VkQueue queue, VkCommandPool command_pool, VkCommandBuffer command_buffer) {
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

/* Buffers and Images */

static inline u32 findMemoryType(u32 type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_prop;
    vkGetPhysicalDeviceMemoryProperties(context->physical_device.handle, &mem_prop);
    for (u32 i = 0; i < mem_prop.memoryTypeCount; ++i) {
        if (type_filter & (1 << i) && (mem_prop.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    platform.log(LOG_INFO, "findMemoryType: failed, couldn't find memory type!");
}

static void createImage(VkDevice device, u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *image_memory) {
    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent = {
            .width = width,
            .height = height,
            .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = format,
        .tiling = tiling,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .flags = 0,
    };

    VKC_CHECK(vkCreateImage(context->logical_device.handle, &image_info, NULL, image), "Failed to create image!");

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(context->logical_device.handle, *image, &mem_req);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_req.size,
        .memoryTypeIndex = findMemoryType(mem_req.memoryTypeBits, properties),
    };

    VKC_CHECK(vkAllocateMemory(context->logical_device.handle, &alloc_info, NULL, image_memory), "Failed to allocate image memory");

    vkBindImageMemory(context->logical_device.handle, *image, *image_memory, 0);
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
        .memoryTypeIndex = findMemoryType(mem_req.memoryTypeBits, properties),
    };

    VKC_CHECK(vkAllocateMemory(device, &alloc_info, NULL, memory), "failed to allocate memory for buffer");

    vkBindBufferMemory(device, *buffer, *memory, 0);
}

static void copyBuffer(VkDevice device, VkQueue queue, VkCommandPool command_pool, VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBuffer command_buffer = beginSingleTimeCommands(device, command_pool);

    VkBufferCopy copy_region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };
    vkCmdCopyBuffer(command_buffer, src, dst, 1, &copy_region);

    endSingleTimeCommands(device, queue, command_pool, command_buffer);
}

static inline VkImageView createImageView(VkImage image, VkFormat format) {
    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    VkImageView view;
    VKC_CHECK(vkCreateImageView(context->logical_device.handle, &view_info, NULL, &view), "Failed to create texture image view");
    return view;
}

static void copyBufferToImage(VkDevice device, VkQueue queue, VkCommandPool command_pool, VkBuffer buffer, VkImage image, u32 width, u32 height) {
    VkCommandBuffer command_buffer = beginSingleTimeCommands(device, command_pool);

    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {
            .width = width,
            .height = height,
            .depth = 1,
        },
    };

    vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(device, queue, command_pool, command_buffer);
}

static void transitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool command_pool, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, u32 src_access, u32 dst_access) {
    VkCommandBuffer command_buffer = beginSingleTimeCommands(device, command_pool);

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .srcAccessMask = src_access,
        .dstAccessMask = dst_access,
    };

    vkCmdPipelineBarrier(command_buffer,
                         src_stage, dst_stage,
                         0,
                         0, NULL,
                         0, NULL,
                         1, &barrier);

    endSingleTimeCommands(device, queue, command_pool, command_buffer);
}

/* Descriptor sets and pools */

static void create_descriptor_set_layout(VkDevice device, VkDescriptorSetLayout *descriptor_set_layout) {
    //VkDescriptorSetLayoutBinding ubo_layout_binding = {
    //    .binding = 0,
    //    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    //    .descriptorCount = 1,
    //    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    //    .pImmutableSamplers = NULL,
    //};

    VkDescriptorSetLayoutBinding sampler_layout_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = NULL,
    };

    VkDescriptorSetLayoutBinding layout_bindings[] = {
        //[0] = ubo_layout_binding,
        [0] = sampler_layout_binding,
    };

    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = ARRLEN(layout_bindings),
        .pBindings = layout_bindings,
    };

    VKC_CHECK(vkCreateDescriptorSetLayout(device, &layout_info, NULL, descriptor_set_layout), "failed to create descriptor set layout");
}

static void create_descriptor_pool(VkDevice device, u32 count, VkDescriptorPool *descriptor_pool) {
    VkDescriptorPoolSize sizes[] = {
        [0] = {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = count,
        },
        [1] = {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = count,
        },
    };

    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = ARRLEN(sizes),
        .pPoolSizes = sizes,
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

/* Swapchain */

/* TODO(anjo): a lot of this should probably be handled in the selection of a physical device */
bool getSwapchainInfo(VkSurfaceKHR surface, struct vkc_physical_device *physical_device, SwapchainInfo *info) {
    platform.log(LOG_INFO, "Vulkan: Getting swapchain info");

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device->handle, surface, &info->capabilities);

    u32 vk_format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device->handle, surface, &vk_format_count, NULL);
    VkSurfaceFormatKHR vk_formats[vk_format_count];
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device->handle, surface, &vk_format_count, vk_formats);
    bool format_found = false;
    for (u32 i = 0; i < vk_format_count; ++i) {
        platform.log(LOG_INFO, "format: %d space %d", vk_formats[i].format, vk_formats[i].colorSpace);
        if ((vk_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM ||
             vk_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) &&
            vk_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
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

void createSwapchain(VkSurfaceKHR surface, struct vkc_logical_device *logical_device, SwapchainInfo *info, u32 width, u32 height, Swapchain *swapchain) {
    swapchain->image_format = info->surface_format.format;
    swapchain->image_color_space = info->surface_format.colorSpace;
    if (info->capabilities.currentExtent.width != UINT32_MAX) {
        swapchain->image_extent = info->capabilities.currentExtent;
    } else {
        swapchain->image_extent = (VkExtent2D) {
            CLAMP(width,  info->capabilities.minImageExtent.width,  info->capabilities.maxImageExtent.width),
            CLAMP(height, info->capabilities.minImageExtent.height, info->capabilities.maxImageExtent.height),
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
        create_info.queueFamilyIndexCount = ARRLEN(queue_indices);
        create_info.pQueueFamilyIndices = queue_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = NULL;
    }

    VKC_CHECK(vkCreateSwapchainKHR(logical_device->handle, &create_info, NULL, &swapchain->handle),
             "failed to create swapchain");

    vkGetSwapchainImagesKHR(logical_device->handle, swapchain->handle, &image_count, NULL);
    swapchain->image_count = image_count;
    swapchain->images = platform.allocate_memory(sizeof(VkImage) * image_count);
    vkGetSwapchainImagesKHR(logical_device->handle, swapchain->handle, &image_count, swapchain->images);
}

void destroySwapchain(struct vkc_logical_device *logical_device, Swapchain *swapchain) {
    platform.free_memory(swapchain->images);
    vkDestroySwapchainKHR(logical_device->handle, swapchain->handle, NULL);
}

void createSwapchainImageViews(struct vkc_logical_device *logical_device, Swapchain *swapchain) {
    swapchain->image_view_count = swapchain->image_count;
    swapchain->image_views = platform.allocate_memory(sizeof(VkImageView) * swapchain->image_view_count);

    for (u32 i = 0; i < swapchain->image_view_count; ++i) {
        swapchain->image_views[i] = createImageView(swapchain->images[i], swapchain->image_format);
    }
}

void destroySwapchainImageViews(struct vkc_logical_device *logical_device, Swapchain *swapchain) {
    for (u32 i = 0; i < swapchain->image_view_count; ++i) {
        vkDestroyImageView(logical_device->handle, swapchain->image_views[i], NULL);
    }
    platform.free_memory(swapchain->image_views);
}

/* Framebuffer */

VkFramebuffer *vkc_create_framebuffers(VkDevice device, VkRenderPass renderpass, Swapchain *swapchain) {
    VkFramebuffer *fbs = platform.allocate_memory(sizeof(VkFramebuffer) * swapchain->image_view_count);

    for (u32 i = 0; i < swapchain->image_view_count; ++i) {
        VkImageView attachments[] = {
            swapchain->image_views[i]
        };

        VkFramebufferCreateInfo fb_info = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = renderpass,
            .attachmentCount = 1,
            .pAttachments    = attachments,
            .width           = swapchain->image_extent.width,
            .height          = swapchain->image_extent.height,
            .layers          = 1,
        };

        VKC_CHECK(vkCreateFramebuffer(device, &fb_info, NULL, &fbs[i]),
                 "failed to create frame buffer");
    }

    return fbs;
}

void vkc_destroy_framebuffers(VkDevice device, VkFramebuffer *fbs, u32 fb_count) {
    for (u32 i = 0; i < fb_count; ++i) {
        vkDestroyFramebuffer(device, fbs[i], NULL);
    }

    platform.free_memory(fbs);
}

/* Renderpass */

VkRenderPass vkc_create_renderpass(VkDevice device, Swapchain *swapchain) {
    VkSubpassDependency dependency = {
        .srcSubpass    = VK_SUBPASS_EXTERNAL,
        .dstSubpass    = 0,
        .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkAttachmentDescription color_attachment = {
        .format         = swapchain->image_format,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &color_attachment_ref,
    };

    VkRenderPassCreateInfo render_pass_info = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments    = &color_attachment,
        .subpassCount    = 1,
        .pSubpasses      = &subpass,
        .dependencyCount = 1,
        .pDependencies   = &dependency,
    };

    VkRenderPass pass = VK_NULL_HANDLE;
    VKC_CHECK(vkCreateRenderPass(device, &render_pass_info, NULL, &pass),
             "failed to create render pass");

    return pass;
}

/* Pipeline */

VkShaderModule create_shader_module(VkDevice device, const char *code, u64 code_size) {
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code_size,
        .pCode = (const u32*) code,
    };

    VkShaderModule shader_module;
    VKC_CHECK(vkCreateShaderModule(device, &create_info, NULL, &shader_module),
             "failed to create shader module");

    return shader_module;
}

struct vkc_pipeline create_pipeline(VkDevice device, VkRenderPass renderpass, Swapchain *swapchain, VkShaderModule vert_module, VkShaderModule frag_module, VkVertexInputBindingDescription vertex_binding_desc, VkVertexInputAttributeDescription* vertex_attrib_desc, u32 attrib_count, VkDescriptorSetLayout *descriptor_set_layout, VkPushConstantRange *push_constant) {

    // Create the pipeline layout

    const u32 set_layout_count = (descriptor_set_layout != NULL) ? 1 : 0;
    const u32 push_count = (push_constant != NULL) ? 1 : 0;
    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = set_layout_count,
        .pSetLayouts            = descriptor_set_layout,
        .pushConstantRangeCount = push_count,
        .pPushConstantRanges    = push_constant,
    };
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VKC_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_info, NULL, &layout),
             "failed to create pipeline layout");

    // Create the pipeline stages

    VkPipelineShaderStageCreateInfo vert_create_info = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vert_module,
        .pName  = "main",
    };

    VkPipelineShaderStageCreateInfo frag_create_info = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = frag_module,
        .pName  = "main",
    };

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_create_info, frag_create_info};
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = 1,
        .pVertexBindingDescriptions      = &vertex_binding_desc,
        .vertexAttributeDescriptionCount = attrib_count,
        .pVertexAttributeDescriptions    = vertex_attrib_desc,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width  = (f32) swapchain->image_extent.width,
        .height = (f32) swapchain->image_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .offset = (VkOffset2D) { .x = 0, .y = 0 },
        .extent = swapchain->image_extent,
    };

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports    = &viewport,
        .scissorCount  = 1,
        .pScissors     = &scissor,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .lineWidth               = 1.0f,
        .cullMode                = VK_CULL_MODE_BACK_BIT,
        .frontFace               = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp          = 0.0f,
        .depthBiasSlopeFactor    = 0.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable   = VK_FALSE,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading      = 1.0f,
        .pSampleMask           = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable      = VK_FALSE,
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT
                             | VK_COLOR_COMPONENT_G_BIT
                             | VK_COLOR_COMPONENT_B_BIT
                             | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable         = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
    };

    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable     = VK_FALSE,
        .logicOp           = VK_LOGIC_OP_COPY,
        .attachmentCount   = 1,
        .pAttachments      = &color_blend_attachment,
        .blendConstants[0] = 0.0f,
        .blendConstants[1] = 0.0f,
        .blendConstants[2] = 0.0f,
        .blendConstants[3] = 0.0f,
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount          = 2,
        .pStages             = shader_stages,
        .pVertexInputState   = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState      = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState   = &multisampling,
        .pDepthStencilState  = NULL,
        .pColorBlendState    = &color_blending,
        .pDynamicState       = NULL,
        .layout              = layout,
        .renderPass          = renderpass,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
    };

    VkPipeline pipeline = VK_NULL_HANDLE;
    VKC_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline),
             "failed to create graphics pipeline");

    return (struct vkc_pipeline) {pipeline, layout};
}

void vkc_destroy_pipeline(VkDevice device, struct vkc_pipeline pipeline) {
    vkDestroyPipeline(device, pipeline.handle, NULL);
    vkDestroyPipelineLayout(device, pipeline.layout, NULL);
}

/*
 * Use of vulkan to create the renderer
 */

static void createTextureSampler() {
    VkPhysicalDeviceProperties properties = {0};
    vkGetPhysicalDeviceProperties(context->physical_device.handle, &properties);

    VkSamplerCreateInfo sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        /* NOTE(anjo): for 2d linear is probably better */
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        /* NOTE(anjo): we probably don't need this for 2d */
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias = 0.0f,
        .minLod = 0.0f,
        .maxLod = 0.0f,
    };

    VKC_CHECK(vkCreateSampler(context->logical_device.handle, &sampler_info, NULL, &context->texture_sampler), "Failed to create texture sampler");
}

static void createTextureImageView() {
    context->texture_image_view = createImageView(context->texture_image, VK_FORMAT_R8_SRGB);
}

static void createTextureImage(Image *image) {
    VkDeviceSize size = image->width * image->height * image->channels;
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;

    create_buffer(context->logical_device.handle, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);

    void *data;
    vkMapMemory(context->logical_device.handle, staging_buffer_memory, 0, size, 0, &data);
    memcpy(data, image->pixels, size);
    vkUnmapMemory(context->logical_device.handle, staging_buffer_memory);

    createImage(context->logical_device.handle, image->width, image->height, VK_FORMAT_R8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &context->texture_image, &context->texture_image_memory);

    transitionImageLayout(context->logical_device.handle, context->logical_device.graphics_queue, context->command_pool, context->texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
    copyBufferToImage(context->logical_device.handle, context->logical_device.graphics_queue, context->command_pool, staging_buffer, context->texture_image, image->width, image->height);
    transitionImageLayout(context->logical_device.handle, context->logical_device.graphics_queue, context->command_pool, context->texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

    vkDestroyBuffer(context->logical_device.handle, staging_buffer, NULL);
    vkFreeMemory(context->logical_device.handle, staging_buffer_memory, NULL);
}

static void create_uniform_buffers(VkDevice device, VkDeviceSize size, VkBuffer *buffers, VkDeviceMemory *memory, u32 count) {
    for (u32 i = 0; i < count; ++i) {
        create_buffer(device, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffers[i], &memory[i]);
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
    // TODO(anjo): why does cleanup swapchain destroy renderpasses and pipelines? Only our eternal lord knows
    vkc_destroy_pipeline(context->logical_device.handle, context->color_pipeline);
    vkc_destroy_pipeline(context->logical_device.handle, context->texture_pipeline);
    vkc_destroy_pipeline(context->logical_device.handle, context->atlas_pipeline);
    vkDestroyRenderPass(context->logical_device.handle, context->renderpass, NULL);
    destroySwapchainImageViews(&context->logical_device, &context->swapchain);
    destroySwapchain(&context->logical_device, &context->swapchain);
}

static void populate_descriptor_sets() {
    for (u32 i = 0; i < context->swapchain.image_count; ++i) {
        //VkDescriptorBufferInfo buffer_info = {
        //    .buffer = context->uniform_buffers[i],
        //    .offset = 0,
        //    .range = sizeof(struct uniform_buffer_object),
        //};

        VkDescriptorImageInfo image_info = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = context->texture_image_view,
            .sampler = context->texture_sampler,
        };

        VkWriteDescriptorSet writes[] = {
            //[0] = {
            //    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            //    .dstSet = context->descriptor_sets[i],
            //    .dstBinding = 0,
            //    .dstArrayElement = 0,
            //    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            //    .descriptorCount = 1,
            //    .pBufferInfo = &buffer_info,
            //},
            [0] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = context->descriptor_sets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .pImageInfo = &image_info,
            },
        };

        vkUpdateDescriptorSets(context->logical_device.handle, ARRLEN(writes), writes, 0, NULL);
    }
}

static void recreate_swapchain(Renderer *r) {
    VKC_CHECK(vkDeviceWaitIdle(context->logical_device.handle), "wait idle failed");

    cleanup_swapchain();

    SwapchainInfo swapchain_info = {0};
    getSwapchainInfo(context->surface, &context->physical_device, &swapchain_info);
    int width = 0, height = 0;
    glfwGetFramebufferSize(r->window, &width, &height);
    createSwapchain(context->surface, &context->logical_device, &swapchain_info, width, height, &context->swapchain);
    createSwapchainImageViews(&context->logical_device, &context->swapchain);
    context->renderpass = vkc_create_renderpass(context->logical_device.handle, &context->swapchain);

    {
        VkVertexInputBindingDescription binding_description = {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };

        VkVertexInputAttributeDescription attribute_descriptions[] = {
            [0] = {
                .binding = 0,
                .location = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, pos),
            },
        };

        VkPushConstantRange push_constant = {
            .offset = 0,
            .size = sizeof(ShapePushConstants),
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };

        context->color_pipeline = create_pipeline(context->logical_device.handle,
                                                  context->renderpass,
                                                  &context->swapchain,
                                                  context->color_vert_module,
                                                  context->color_frag_module,
                                                  binding_description,
                                                  attribute_descriptions,
                                                  ARRLEN(attribute_descriptions),
                                                  NULL,
                                                  &push_constant);
    }

    create_descriptor_pool(context->logical_device.handle, context->swapchain.image_count, &context->descriptor_pool);
    context->descriptor_sets = platform.allocate_memory(sizeof(VkDescriptorSet) * context->swapchain.image_count);
    create_descriptor_sets(context->logical_device.handle, context->swapchain.image_count, &context->descriptor_pool, context->descriptor_set_layout, context->descriptor_sets);

    {

        VkVertexInputBindingDescription binding_description = {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };

        VkVertexInputAttributeDescription attribute_descriptions[] = {
            [0] = {
                .binding = 0,
                .location = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, pos),
            },
            [1] = {
                .binding = 0,
                .location = 1,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, texcoord),
            },
        };

        VkPushConstantRange push_constant = {
            .offset = 0,
            .size = sizeof(ShapePushConstants),
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };

        context->texture_pipeline = create_pipeline(context->logical_device.handle,
                                                    context->renderpass,
                                                    &context->swapchain,
                                                    context->texture_vert_module,
                                                    context->texture_frag_module,
                                                    binding_description,
                                                    attribute_descriptions,
                                                    ARRLEN(attribute_descriptions),
                                                    &context->descriptor_set_layout,
                                                    &push_constant);
    }

    {
        VkVertexInputBindingDescription binding_description = {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };

        VkVertexInputAttributeDescription attribute_descriptions[] = {
            [0] = {
                .binding = 0,
                .location = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, pos),
            },
            [1] = {
                .binding = 0,
                .location = 1,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, texcoord),
            },
        };

        VkPushConstantRange push_constant = {
            .offset = 0,
            .size = sizeof(AtlasPushConstants),
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };

        context->atlas_pipeline = create_pipeline(context->logical_device.handle,
                                                    context->renderpass,
                                                    &context->swapchain,
                                                    context->atlas_vert_module,
                                                    context->atlas_frag_module,
                                                    binding_description,
                                                    attribute_descriptions,
                                                    ARRLEN(attribute_descriptions),
                                                    &context->descriptor_set_layout,
                                                    &push_constant);
    }

    /* framebuffer */
    context->framebuffers = vkc_create_framebuffers(context->logical_device.handle, context->renderpass, &context->swapchain);
    context->framebuffer_count = context->swapchain.image_view_count;

    create_uniform_buffers(context->logical_device.handle, sizeof(struct uniform_buffer_object), context->uniform_buffers, context->uniform_buffers_memory, context->swapchain.image_count);

    if (context->has_texture) {
        populate_descriptor_sets();
    }

    /* command buffer */
    context->command_buffers = vkc_create_command_buffers(context->logical_device.handle, context->command_pool, context->swapchain.image_count);

    for (u32 i = 0; i < context->swapchain.image_count; ++i) {
        context->in_flight_images[i] = VK_NULL_HANDLE;
    }
}

static inline void setup_globals(Renderer *r) {
    platform = r->platform;
    context = r->context;
}

static inline void initialize_vulkan(Renderer *r) {
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

    SwapchainInfo swapchain_info = {0};
    getSwapchainInfo(context->surface, &context->physical_device, &swapchain_info);
    int width = 0, height = 0;
    glfwGetFramebufferSize(r->window, &width, &height);
    createSwapchain(context->surface, &context->logical_device, &swapchain_info, width, height, &context->swapchain);
    createSwapchainImageViews(&context->logical_device, &context->swapchain);
    context->renderpass = vkc_create_renderpass(context->logical_device.handle, &context->swapchain);

    {
        u8 *vert_code = NULL;
        u64 vert_code_size = 0;
        platform.read_file_to_buffer("res/color.vert.spv", &vert_code, &vert_code_size);
        context->color_vert_module = create_shader_module(context->logical_device.handle, vert_code, vert_code_size);
        platform.free_memory(vert_code);

        u8 *frag_code = NULL;
        u64 frag_code_size = 0;
        platform.read_file_to_buffer("res/color.frag.spv", &frag_code, &frag_code_size);
        context->color_frag_module = create_shader_module(context->logical_device.handle, frag_code, frag_code_size);
        platform.free_memory(frag_code);

        VkVertexInputBindingDescription binding_description = {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };

        VkVertexInputAttributeDescription attribute_descriptions[] = {
            [0] = {
                .binding = 0,
                .location = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, pos),
            },
        };


        VkPushConstantRange push_constant = {
            .offset = 0,
            .size = sizeof(ShapePushConstants),
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };

        context->color_pipeline = create_pipeline(context->logical_device.handle,
                                                  context->renderpass,
                                                  &context->swapchain,
                                                  context->color_vert_module,
                                                  context->color_frag_module,
                                                  binding_description,
                                                  attribute_descriptions,
                                                  ARRLEN(attribute_descriptions),
                                                  NULL,
                                                  &push_constant);
    }

    create_descriptor_set_layout(context->logical_device.handle, &context->descriptor_set_layout);
    create_descriptor_pool(context->logical_device.handle, context->swapchain.image_count, &context->descriptor_pool);
    context->descriptor_sets = platform.allocate_memory(sizeof(VkDescriptorSet) * context->swapchain.image_count);
    create_descriptor_sets(context->logical_device.handle, context->swapchain.image_count, &context->descriptor_pool, context->descriptor_set_layout, context->descriptor_sets);

    {
        u8 *vert_code = NULL;
        u64 vert_code_size = 0;
        platform.read_file_to_buffer("res/texture.vert.spv", &vert_code, &vert_code_size);
        context->texture_vert_module = create_shader_module(context->logical_device.handle, vert_code, vert_code_size);
        platform.free_memory(vert_code);

        u8 *frag_code = NULL;
        u64 frag_code_size = 0;
        platform.read_file_to_buffer("res/texture.frag.spv", &frag_code, &frag_code_size);
        context->texture_frag_module = create_shader_module(context->logical_device.handle, frag_code, frag_code_size);
        platform.free_memory(frag_code);

        VkVertexInputBindingDescription binding_description = {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };

        VkVertexInputAttributeDescription attribute_descriptions[2] = {
            [0] = {
                .binding = 0,
                .location = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, pos),
            },
            [1] = {
                .binding = 0,
                .location = 1,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, texcoord),
            },
        };

        VkPushConstantRange push_constant = {
            .offset = 0,
            .size = sizeof(ShapePushConstants),
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };

        context->texture_pipeline = create_pipeline(context->logical_device.handle,
                                                    context->renderpass,
                                                    &context->swapchain,
                                                    context->texture_vert_module,
                                                    context->texture_frag_module,
                                                    binding_description,
                                                    attribute_descriptions,
                                                    ARRLEN(attribute_descriptions),
                                                    &context->descriptor_set_layout,
                                                    &push_constant);
    }

    {
        u8 *vert_code = NULL;
        u64 vert_code_size = 0;
        platform.read_file_to_buffer("res/atlas.vert.spv", &vert_code, &vert_code_size);
        context->atlas_vert_module = create_shader_module(context->logical_device.handle, vert_code, vert_code_size);
        platform.free_memory(vert_code);

        u8 *frag_code = NULL;
        u64 frag_code_size = 0;
        platform.read_file_to_buffer("res/atlas.frag.spv", &frag_code, &frag_code_size);
        context->atlas_frag_module = create_shader_module(context->logical_device.handle, frag_code, frag_code_size);
        platform.free_memory(frag_code);

        VkVertexInputBindingDescription binding_description = {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };

        VkVertexInputAttributeDescription attribute_descriptions[2] = {
            [0] = {
                .binding = 0,
                .location = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, pos),
            },
            [1] = {
                .binding = 0,
                .location = 1,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, texcoord),
            },
        };

        VkPushConstantRange push_constant = {
            .offset = 0,
            .size = sizeof(AtlasPushConstants),
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };

        context->atlas_pipeline = create_pipeline(context->logical_device.handle,
                                                  context->renderpass,
                                                  &context->swapchain,
                                                  context->atlas_vert_module,
                                                  context->atlas_frag_module,
                                                  binding_description,
                                                  attribute_descriptions,
                                                  ARRLEN(attribute_descriptions),
                                                  &context->descriptor_set_layout,
                                                  &push_constant);
    }

    /* framebuffer */
    context->framebuffers = vkc_create_framebuffers(context->logical_device.handle, context->renderpass, &context->swapchain);
    context->framebuffer_count = context->swapchain.image_view_count;

    /* command buffer */
    context->command_pool = vkc_create_command_pool(context->logical_device.handle, context->physical_device.graphics_family);
    context->command_buffers = vkc_create_command_buffers(context->logical_device.handle, context->command_pool, context->swapchain.image_count);

    context->uniform_buffers = platform.allocate_memory(sizeof(VkBuffer) * context->swapchain.image_count);
    context->uniform_buffers_memory = platform.allocate_memory(sizeof(VkDeviceMemory) * context->swapchain.image_count);
    create_uniform_buffers(context->logical_device.handle, sizeof(struct uniform_buffer_object), context->uniform_buffers, context->uniform_buffers_memory, context->swapchain.image_count);

    createTextureSampler();

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

    // NOTE(anjo): we do not have seperate vertices for the textured vs colored case.
    //             The UV coord. are always passed.

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
        copyBuffer(context->logical_device.handle, context->logical_device.graphics_queue, context->command_pool, staging_buffer, context->vertex_buffer, size);

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
        copyBuffer(context->logical_device.handle, context->logical_device.graphics_queue, context->command_pool, staging_buffer, context->index_buffer, size);

        vkDestroyBuffer(context->logical_device.handle, staging_buffer, NULL);
        vkFreeMemory(context->logical_device.handle, staging_buffer_memory, NULL);
    }
}

static inline void initialize_fonts(Renderer *r) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        platform.log(LOG_ERROR, "FreeType: Could not init!");
        return;
    }

    u8 *font = NULL;
    u64 size = 0;
    platform.read_file_to_buffer("res/fonts/lmroman10-regular.otf", &font, &size);

    FT_Open_Args args = {
        .flags = FT_OPEN_MEMORY,
        .memory_base = font,
        .memory_size = size,
        .pathname = NULL,
        .stream = 0,
        .driver = NULL,
        .num_params = 0,
        .params = NULL,
    };

    FT_Face face;
    if (FT_Open_Face(ft, &args, 0, &face)) {
        platform.log(LOG_ERROR, "FreeType: Could not open face!");
        return;
    }

    platform.free_memory(font);

    FT_Set_Pixel_Sizes(face, 0, 100);

    if (FT_Load_Char(face, 'A', FT_LOAD_RENDER)) {
        platform.log(LOG_ERROR, "FreeType: Could not load glyph!");
        return;
    }
}

/* Interface to loader */

void startup(Renderer *r) {
    r->context = r->platform.allocate_memory(sizeof(RenderContext));
    memset(r->context, 0, sizeof(RenderContext));
    setup_globals(r);

    initialize_vulkan(r);
    //initialize_fonts(r);
}

void shutdown(Renderer *r) {
    setup_globals(r);
    vkDeviceWaitIdle(context->logical_device.handle);

    cleanup_swapchain(r);

    vkDestroyShaderModule(context->logical_device.handle, context->atlas_vert_module, NULL);
    vkDestroyShaderModule(context->logical_device.handle, context->atlas_frag_module, NULL);
    vkDestroyShaderModule(context->logical_device.handle, context->texture_vert_module, NULL);
    vkDestroyShaderModule(context->logical_device.handle, context->texture_frag_module, NULL);
    vkDestroyShaderModule(context->logical_device.handle, context->color_vert_module, NULL);
    vkDestroyShaderModule(context->logical_device.handle, context->color_frag_module, NULL);

    vkDestroySampler(context->logical_device.handle, context->texture_sampler, NULL);

    if (context->has_texture) {
        vkDestroyImageView(context->logical_device.handle, context->texture_image_view, NULL);
        vkDestroyImage(context->logical_device.handle, context->texture_image, NULL);
        vkFreeMemory(context->logical_device.handle, context->texture_image_memory, NULL);
    }

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

    VKC_CHECK(vkBeginCommandBuffer(context->command_buffers[image_index], &cmd_info),
              "failed to start recording command buffer");
    pass_info.framebuffer = context->framebuffers[image_index];
    vkCmdBeginRenderPass(context->command_buffers[image_index], &pass_info, VK_SUBPASS_CONTENTS_INLINE);

    // TODO(anjo): Move to separate queues for different pipelines?

    RenderEntryHeader *header = (RenderEntryHeader *) cmds->memory_base;
    while ((u8 *)header < cmds->memory_base + cmds->memory_top) {
        switch (header->type) {
        case ENTRY_TYPE_RenderEntryQuad: {
            RenderEntryQuad *quad = (RenderEntryQuad *) header;
            header += sizeof(RenderEntryQuad);

            vkCmdBindPipeline(context->command_buffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, context->color_pipeline.handle);
            VkBuffer vertex_buffers[] = {context->vertex_buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(context->command_buffers[image_index], 0, 1, vertex_buffers, offsets);
            vkCmdBindIndexBuffer(context->command_buffers[image_index], context->index_buffer, 0, VK_INDEX_TYPE_UINT16);

            /* Draw quad */
            ShapePushConstants push = {0};
            v2AssignToArray(push.pos, quad->pos);
            v2AssignToArray(push.scale, quad->scale);
            colorRGBAssignToArray(push.col, quad->col);
            vkCmdPushConstants(context->command_buffers[image_index], context->color_pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShapePushConstants), &push);

            vkCmdDrawIndexed(context->command_buffers[image_index], ARRLEN(indices), 1, 0, 0, 0);

            break;
        }
        case ENTRY_TYPE_RenderEntryTexturedQuad: {
            RenderEntryTexturedQuad *quad = (RenderEntryTexturedQuad *) header;
            header += sizeof(RenderEntryTexturedQuad);

            if (!context->has_texture) {
                createTextureImage(&quad->image);
                createTextureImageView();
                populate_descriptor_sets();
                context->has_texture = true;
            }

            vkCmdBindPipeline(context->command_buffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, context->texture_pipeline.handle);
            VkBuffer vertex_buffers[] = {context->vertex_buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(context->command_buffers[image_index], 0, 1, vertex_buffers, offsets);
            vkCmdBindIndexBuffer(context->command_buffers[image_index], context->index_buffer, 0, VK_INDEX_TYPE_UINT16);

            vkCmdBindDescriptorSets(context->command_buffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, context->texture_pipeline.layout, 0, 1, &context->descriptor_sets[image_index], 0, NULL);

            /* Draw quad */
            ShapePushConstants push = {0};
            v2AssignToArray(push.pos, quad->pos);
            v2AssignToArray(push.scale, quad->scale);
            //colorRGBAssignToArray(push.col, quad->col);
            vkCmdPushConstants(context->command_buffers[image_index], context->texture_pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShapePushConstants), &push);

            vkCmdDrawIndexed(context->command_buffers[image_index], ARRLEN(indices), 1, 0, 0, 0);

            break;
        }
        case ENTRY_TYPE_RenderEntryAtlasQuad: {
            RenderEntryAtlasQuad *quad = (RenderEntryAtlasQuad *) header;
            header += sizeof(RenderEntryAtlasQuad);

            if (!context->has_texture) {
                createTextureImage(&quad->image);
                createTextureImageView();
                populate_descriptor_sets();
                context->has_texture = true;
            }

            vkCmdBindPipeline(context->command_buffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, context->atlas_pipeline.handle);
            VkBuffer vertex_buffers[] = {context->vertex_buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(context->command_buffers[image_index], 0, 1, vertex_buffers, offsets);
            vkCmdBindIndexBuffer(context->command_buffers[image_index], context->index_buffer, 0, VK_INDEX_TYPE_UINT16);

            vkCmdBindDescriptorSets(context->command_buffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, context->atlas_pipeline.layout, 0, 1, &context->descriptor_sets[image_index], 0, NULL);

            /* Draw quad */
            AtlasPushConstants push = {0};
            v2AssignToArray(push.pos, quad->pos);
            v2AssignToArray(push.scale, quad->scale);
            v2AssignToArray(push.offset, quad->offset);
            v2AssignToArray(push.size, quad->size);
            colorRGBAssignToArray(push.col, quad->col);
            vkCmdPushConstants(context->command_buffers[image_index], context->atlas_pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(AtlasPushConstants), &push);

            vkCmdDrawIndexed(context->command_buffers[image_index], ARRLEN(indices), 1, 0, 0, 0);

            break;
        }
        case ENTRY_TYPE_RenderEntryText: {
            RenderEntryText *entry = (RenderEntryText *) header;
            header += sizeof(RenderEntryText);

            Vec2 pos = entry->pos;

            for (const char *p = entry->text; *p; ++p) {

                PackRect *rect = NULL;
                for (u32 i = 0; i < NUM_CHARS; ++i) {
                    if (r->font_map[i].user_id == *p) {
                        rect = &r->font_map[i];
                        break;
                    }
                }

                FontInfo *info = NULL;
                for (u32 i = 0; i < NUM_CHARS; ++i) {
                    if (r->font_info[i].codepoint == *p) {
                        info = &r->font_info[i];
                        break;
                    }
                }

                if (rect) {
                    pushAtlasQuad(cmds,
                                  VEC2(pos.x + 2.0f*(info->offset_x)/800.0f, pos.y - 2.0f*(info->offset_y)/600.0f),
                                  VEC2(2.0f*rect->width/800.0f, 2.0f*rect->height/600.0f),
                                  *r->font_atlas,
                                  VEC2(rect->x/(f32)r->font_atlas->width,     rect->y/(f32)r->font_atlas->height),
                                  VEC2(rect->width/(f32)r->font_atlas->width, rect->height/(f32)r->font_atlas->height),
                                  entry->col
                                  );
                    pos.x += 2.0*(f32)(info->advance >> 6)/800.0f;
                }
            }

            break;
        }
        };
    }

    vkCmdEndRenderPass(context->command_buffers[image_index]);
    VKC_CHECK(vkEndCommandBuffer(context->command_buffers[image_index]),
              "failed to end recording command buffer");

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
