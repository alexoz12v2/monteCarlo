#ifndef MXC_VULKAN_COMMONS_H
#define MXC_VULKAN_COMMONS_H

#include <vulkan/vulkan.h>

#include <cassert>
#include <cstdint>
#define VK_CHECK(result) assert(result == VK_SUCCESS)

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
}

#endif // MXC_VULAKN_COMMONS_H
