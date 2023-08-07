#ifndef MXC_VULKAN_COMMONS_H
#define MXC_VULKAN_COMMONS_H

#include <vulkan/vulkan.h>

#include <cassert>
#include <cstdint>
#define VK_CHECK(result) [&]()->VkResult{VkResult res = (result); assert(res == VK_SUCCESS); return res;}()

namespace mxc
{

	// stuff the renderer passes to all related vulkan classes by reference to perform their operation
	struct VulkanContext;

	enum class CommandType : uint8_t
	{
		GRAPHICS,
		TRANSFER,
		COMPUTE
	};

	// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkResult.html, only some
	auto vulkanResultToString(VkResult) -> char const*;

	inline auto clamp(float value, float min, float max) -> float
	{
		return  value < min ? min :
			value > max ? max :
			value;
	}


    inline auto vulkanResultToString(VkResult res) -> char const*
    {
        if(res == VK_SUCCESS) return "VK_SUCCESS";
        if(res == VK_NOT_READY) return "VK_NOT_READY";
        if(res == VK_TIMEOUT) return "VK_TIMEOUT";
        if(res == VK_EVENT_SET) return "VK_EVENT_SET";
        if(res == VK_EVENT_RESET) return "VK_EVENT_RESET";
        if(res == VK_INCOMPLETE) return "VK_INCOMPLETE";
        if(res == VK_ERROR_OUT_OF_HOST_MEMORY) return "VK_ERROR_OUT_OF_HOST_MEMORY";
        if(res == VK_ERROR_OUT_OF_DEVICE_MEMORY) return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        if(res == VK_ERROR_INITIALIZATION_FAILED) return "VK_ERROR_INITIALIZATION_FAILED";
        if(res == VK_ERROR_DEVICE_LOST) return "VK_ERROR_DEVICE_LOST";
        if(res == VK_ERROR_MEMORY_MAP_FAILED) return "VK_ERROR_MEMORY_MAP_FAILED";
        if(res == VK_ERROR_LAYER_NOT_PRESENT) return "VK_ERROR_LAYER_NOT_PRESENT";
        if(res == VK_ERROR_EXTENSION_NOT_PRESENT) return "VK_ERROR_EXTENSION_NOT_PRESENT";
        if(res == VK_ERROR_FEATURE_NOT_PRESENT) return "VK_ERROR_FEATURE_NOT_PRESENT";
        if(res == VK_ERROR_INCOMPATIBLE_DRIVER) return "VK_ERROR_INCOMPATIBLE_DRIVER";
        if(res == VK_ERROR_TOO_MANY_OBJECTS) return "VK_ERROR_TOO_MANY_OBJECTS";
        if(res == VK_ERROR_FORMAT_NOT_SUPPORTED) return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        if(res == VK_ERROR_FRAGMENTED_POOL) return "VK_ERROR_FRAGMENTED_POOL";
        if(res == VK_ERROR_UNKNOWN) return "VK_ERROR_UNKNOWN";
        if(res == VK_ERROR_SURFACE_LOST_KHR) return "VK_ERROR_SURFACE_LOST_KHR";
        // Provided by VK_KHR_surface
        if(res == VK_ERROR_NATIVE_WINDOW_IN_USE_KHR) return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        // Provided by VK_KHR_swapchain
        if(res == VK_SUBOPTIMAL_KHR) return "VK_SUBOPTIMAL_KHR";
        // Provided by VK_KHR_swapchain
        if(res == VK_ERROR_OUT_OF_DATE_KHR) return "VK_ERROR_OUT_OF_DATE_KHR";
        return "unknown";
    }
}

#endif // MXC_VULAKN_COMMONS_H
