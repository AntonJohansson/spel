#pragma once

#include <vulkan/vulkan.h>

#define CLAMP(x, lower, higher) \
	((x < lower) ? lower : (x > higher) ? higher : x)

#define ARRLEN(arr) \
	(sizeof(arr)/sizeof(arr[0]))

const char* vk_result_to_string(VkResult result);
char* read_file_to_buffer(const char filename[], uint64_t* size);

const char* vk_result_to_string(VkResult result) {
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
