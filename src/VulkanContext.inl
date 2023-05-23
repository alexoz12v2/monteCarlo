#ifndef MXC_VULKAN_CONTEXT_INL
#define MXC_VULKAN_CONTEXT_INL

#include <vulkan/vulkan.h>

#include "Device.h"
#include "Image.h"
#include "Swapchain.h"
#include "CommandBuffer.h"
#include "Pipeline.h"

// TODO remove vector
#include <vector>

namespace mxc
{
	// Note: might overhaul synchronization mechanisms when I add the compute pass
	struct FrameObjects
	{
		VkCommandBuffer commandBuffer; // Note: maybe to remove or tag CommandBuffers with usage su that we can put graphics ones in here
		VkFence renderCompleteFence;
		VkSemaphore renderCompleteSemaphore;
		VkSemaphore presentCompleteSemaphore;
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

		// TODO maybe to change to some other structure, cause we need command buffers for compute too
		std::vector<CommandBuffer> commandBuffers; // as many as frames in flight, ie maxSwapchainImages-1 in the common case
		std::vector<FrameObjects> syncObjs;

		VkRenderPass renderPass;
		std::vector<VkFramebuffer> presentFramebuffers;
		uint32_t framebufferWidth;
		uint32_t framebufferHeight;
		uint32_t currentFramebufferIndex = 0;
		
		VkFormat depthFormat;
		struct ImageViewPair {Image image; ImageView view;};
		std::vector<ImageViewPair> depthImages; // as many as command buffers. Type to be replaced

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
