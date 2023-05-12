#ifndef MXC_VULKAN_CONTEXT_INL
#define MXC_VULKAN_CONTEXT_INL

#include <vulkan/vulkan.h>

#include "Device.h"
#include "Swapchain.h"

namespace mxc
{
	struct DepthImage
	{
		VkImage handle;
		VkImageView view;
	};

	struct VulkanContext
	{
		// data
		VkInstance instance = VK_NULL_HANDLE;
		Device device;
#if defined(_DEBUG)
		VkDebugUtilsMessengerEXT debugMessenger;
#endif

		VkSurfaceKHR surface = VK_NULL_HANDLE;
		Swapchain swapchain;

		uint32_t framebufferWidth;
		uint32_t framebufferHeight;

		
		VkFormat depthFormat;
		std::vector<DepthImage> depthImages; // as many as command buffers

		// event handlers
		auto resized() -> bool;
	};
}

#endif // MXC_VULKAN_CONTEXT_INL

#if defined(MXC_VULKAN_CONTEXT_INL_IMPLEMENTATION)
namespace mxc
{
	auto VulkanContext::resized() -> bool
	{
		return true;
	}
}
#endif // MXC_VULKAN_CONTEXT_INL_IMPLEMENTATION
