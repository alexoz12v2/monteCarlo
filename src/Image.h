#ifndef MXC_IMAGE_H
#define MXC_IMAGE_H

#include <vulkan/vulkan.h>
#include "VulkanCommon.h"

#include <cstdint>

namespace mxc 
{
	// created From a device
	struct Image
	{
		constexpr Image(VkImageType type, VkExtent3D extent, VkFormat format, VkImageUsageFlagBits usage, uint8_t mipLevels = 1, uint8_t arrayLayers= 1) 
			: extent(extent), type(type), format(format), usage(usage), mipLevels(mipLevels), arrayLayers(arrayLayers) {}

        VkExtent3D extent;
		VkImage handle = VK_NULL_HANDLE;
		VkImageType type;
		VkFormat format;
		VkImageUsageFlagBits usage;

		uint32_t memoryIndex = UINT32_MAX;
		uint32_t allocationIndex = UINT32_MAX;

        uint8_t mipLevels;
        uint8_t arrayLayers;
	};
	
	struct ImageView
	{
		constexpr ImageView(
			VkImageViewType viewType, VkImageAspectFlags aspectMask, 
			uint8_t baseMipLevel = 0, uint8_t levelCount = 1, 
			uint8_t baseArrayLayer = 0, uint8_t layerCount = 1) 
			: viewType(viewType), aspectMask(aspectMask)
			, baseMipLevel(baseMipLevel), levelCount(levelCount)
			, baseArrayLayer(baseArrayLayer), layerCount(layerCount) {}

		// potentially unsafe
		Image const* pImage = nullptr;
		VkImageView handle = VK_NULL_HANDLE;
		VkImageViewType viewType;
		// subresourcerange, but with uint8_t
		VkImageAspectFlags aspectMask; 
		uint8_t baseMipLevel, levelCount, baseArrayLayer, layerCount;

		auto isValid() -> bool { return handle != VK_NULL_HANDLE && pImage && pImage->handle != VK_NULL_HANDLE; }
	};
	
	struct Texture
	{};
}

#endif // MXC_IMAGE_H
