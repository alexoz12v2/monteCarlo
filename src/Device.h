#ifndef MXC_DEVICE_H
#define MXC_DEVICE_H

#include "VulkanCommon.h"

#include <vector>

#include <vulkan/vulkan.h>
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

#include <vk_mem_alloc.h>

namespace mxc 
{
	struct SwapchainSupport
	{
		VkSurfaceCapabilitiesKHR surfCaps;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct PhysicalDeviceQueueFamiliesSupport
	{
		int32_t graphicsFamily = -1;
		int32_t presentFamily = -1;
		int32_t transferFamily = -1;
		int32_t computeFamily = -1;
	};

	struct PhysicalDeviceRequirements
	{
		// required extensions
		std::vector<char const*> extensions;

		// required features TODO 
		// bool samplerAnisotropy ...
	};

	class Device
	{
        static uint32_t constexpr QUEUE_FAMILIES_COUNT = 4;
	public:
		VkDevice logical;
		VkPhysicalDevice physical = VK_NULL_HANDLE;
		SwapchainSupport swapchainSupport;

		struct QueueFamilyIndices
		{
			int32_t graphics;
			int32_t present;
			int32_t transfer;
			int32_t compute;
		};

		union 
		{
			QueueFamilyIndices queueFamilies;
			uint32_t queueFamilyIndices[QUEUE_FAMILIES_COUNT];
		};

		VkQueue graphicsQueue;
		VkQueue presentQueue;
		VkQueue transferQueue;
		VkQueue computeQueue;

		VkCommandPool commandPool; // used for graphics, compute, transfer
		VmaAllocator vmaAllocator;

	public: // maybe heap
		VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceMemoryProperties memory;

	public:
		auto create(VulkanContext* ctx, PhysicalDeviceRequirements const& requirements) -> bool;
		auto destroy(VulkanContext* ctx) -> bool; // to be called after vkDeviceWaitIdle
		auto querySwapchainSupport(VulkanContext* ctx, VkSurfaceKHR surface) -> bool;

	private:
		auto selectPhysicalDevice(VulkanContext* ctx, PhysicalDeviceRequirements const& requirements) -> bool;
		auto checkPhysicalDeviceRequirements(VulkanContext* ctx, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, 
											 PhysicalDeviceRequirements const& requirements, 
											 VkPhysicalDeviceProperties const& properties, 
											 VkPhysicalDeviceFeatures const& features, PhysicalDeviceQueueFamiliesSupport* outFamilySupport, 
											 SwapchainSupport* outSwapchainSupport) const -> bool;
		auto selectDepthFormat() -> bool;
		auto internalQuerySwapchainSupport(VulkanContext* ctx, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, SwapchainSupport* outSwapchainSupport) const -> bool;
	};
}

#endif // MXC_DEVICE_H
