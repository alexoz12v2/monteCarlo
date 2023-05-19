#ifndef MXC_BUFFER_H
#define MXC_BUFFER_H

#include <vulkan/vulkan.h>
#include "VulkanCommon.h"

#include <cstdint>

namespace mxc
{
	// deduces usage flags. Not using enum class because I need bitwise ops
	namespace BufferType_v
	{
		enum T : uint8_t
		{
			INVALID = 0,
			VERTEX = 1<<0,
			INDEX = 1<<1,
			UNIFORM = 1<<2,
			STAGING = 1<<3,
			STORAGE = 1<<4,
			UNIFORM_TEXEL = 1<<5,
			STORAGE_TEXEL = 1<<6
		};
	}

	using BufferType_t = BufferType_v::T;
	
	// needs to be created by on Device and destroyed by the same Device class instance
	struct Buffer
	{
		constexpr Buffer(VkDeviceSize size, BufferType_t type) : size(size), type(type) {}
		
		VkBuffer handle = VK_NULL_HANDLE;
		VkDeviceSize size;
		VkMemoryPropertyFlags memoryPropertyFlags;
		uint32_t allocationIndex = UINT32_MAX;
		uint32_t memoryIndex = UINT32_MAX;
		void* mapped = nullptr;

		BufferType_t type;
	};
}

#endif // MXC_BUFFER_H
