#ifndef MXC_DEVICE_H
#define MXC_DEVICE_H

#include "VulkanCommon.h"

#include <vector>

#include <vulkan/vulkan.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>

namespace mxc 
{
	struct Buffer;
	struct Image;
	struct ImageView;

	class CommandBuffer;
	
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

	enum class BufferMemoryOptions : uint8_t
	{
		TRANSFER_DST = 0,
		SYSTEM_MEMORY = 1,
		DEVICE_LOCAL_HOST_VISIBLE = 2, // Note: not always supported, see https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html#usage_patterns_gpu_only
		GPU_ONLY = 3
	};

	namespace DepthFormatProperties_v
	{
		enum T : uint8_t	
		{
			NONE = 0,
			SUPPORTS_STENCIL = 1
		};
	}

	using DepthFormatProperties_t = DepthFormatProperties_v::T;

	class Device
	{
        static uint32_t constexpr QUEUE_FAMILIES_COUNT = 4;
		static uint32_t constexpr INITIAL_ALLOCATIONS_VECTOR_CAPACITY = 64;
	public:
		VkDevice logical = VK_NULL_HANDLE;
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

		VkCommandPool graphicsCmdPool; // used for graphics, compute, transfer
		VkCommandPool computeCmdPool; // used for graphics, compute, transfer
		VkCommandPool transferCmdPool; // used for graphics, compute, transfer
		VmaAllocator vmaAllocator;

	public: // maybe heap
		VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceMemoryProperties memory;

	public:
		auto create(VulkanContext* ctx, PhysicalDeviceRequirements const& requirements) -> bool;
		auto destroy([[maybe_unused]] VulkanContext* ctx) -> bool; // to be called after vkDeviceWaitIdle
		
		auto createBuffer(Buffer* inOutBuffer, BufferMemoryOptions options = BufferMemoryOptions::GPU_ONLY) -> bool;
		auto copyToBuffer(void const* data, VkDeviceSize size, Buffer* dst) -> bool;
		auto copyBuffer(VulkanContext* ctx, Buffer const* src, Buffer* dst) -> bool;
		auto destroyBuffer(Buffer* inOutBuffer) -> void;

		// TODO when you add overloads to the CommandBuffer allocate and free functions, remove the context parameter
		auto createImage(
			VulkanContext* ctx, 
			VkImageTiling tiling,
			Image* inOutImage, 
			VkImageLayout const* targetLayout = nullptr, 
			ImageView* inOutView = nullptr) -> bool;
		auto insertImageMemoryBarrier(
            VkCommandBuffer cmdBuf,
            VkImage image,
            VkImageLayout oldImageLayout,
            VkImageLayout newImageLayout,
            VkImageSubresourceRange subresourceRange,
            VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) -> void;
		// version with subresourceRange With baseMipLevel = 0, levelCount = 1, baseArrayLayer = 0, layerCount = 1, and aspectMask given
		auto insertImageMemoryBarrier(
            VkCommandBuffer cmdBuf,
            VkImage image,
            VkImageLayout oldImageLayout,
            VkImageLayout newImageLayout,
			VkImageAspectFlags imageAspectMask,
            VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) -> void;
		auto destroyImage(Image* inOutImage) -> void;

		auto createImageView(Image const* pImage, ImageView* pOutView) -> bool;
		auto destroyImageView(ImageView* pOutView) -> void;

		// submits and synchronizes with a fence
		auto flushCommandBuffer(CommandBuffer* pCmdBuf, CommandType type) -> bool;

		auto updateSwapchainSupport(VulkanContext* ctx) -> bool;
		auto selectDepthFormat(VkFormat* outDepthFormat, DepthFormatProperties_t* outFormatProps) const -> bool;

	private:
		auto selectPhysicalDevice(VulkanContext* ctx, PhysicalDeviceRequirements const& requirements) -> bool;
		auto checkPhysicalDeviceRequirements(VulkanContext* ctx, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, 
											 PhysicalDeviceRequirements const& requirements, 
											 VkPhysicalDeviceProperties const& properties, 
											 [[maybe_unused]] VkPhysicalDeviceFeatures const& features, 
											 PhysicalDeviceQueueFamiliesSupport* outFamilySupport, 
											 SwapchainSupport* outSwapchainSupport) const -> bool;
		auto querySwapchainSupport(VulkanContext* ctx, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, 
								   SwapchainSupport* outSwapchainSupport) const -> bool;
	
	private:
		struct TaggedAllocations 
		{
			VmaAllocation allocation;
			bool freed;
		};
		std::vector<TaggedAllocations> m_allocations;
	};
}

#endif // MXC_DEVICE_H
