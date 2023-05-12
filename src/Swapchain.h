#ifndef MXC_SWAPCHAIN_H
#define MXC_SWAPCHAIN_H

#include <vulkan/vulkan.h>

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include <wayland-client.h>
#include <vulkan/vulkan_wayland.h>
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#include <xcb/xcb.h>
#include <vulkan/vulkan_xcb.h>
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
#include <vulkan/vulkan_macos.h>
#endif

// TODO
#include <vector>

#include "VulkanCommon.h"

namespace mxc
{
	struct SwapchainImage
	{
		VkImage handle;
		VkImageView view;
	};
	
	class Swapchain
	{
	public:
		// Sascha Willelms examples
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		auto initSurface(VulkanContext* ctx, void* platformHandle, void* platformWindow) -> bool;
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
		auto initSurface(VulkanContext* ctx, wl_display* display, wl_surface* window) -> bool;
#elif defined(VK_USE_PLATFORM_XCB_KHR)
		auto initSurface(VulkanContext* ctx, xcb_connection_t* connection, xcb_window_t window) -> bool;
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
		auto initSurface(VulkanContext* ctx, void* view) -> bool;
#endif
		auto create(VulkanContext* ctx, uint32_t width, uint32_t height, bool vsync) -> bool;
		auto destroy(VulkanContext* ctx) -> void; // call only if vkDeviceIdle

		auto initFunctionPointers(VulkanContext* ctx) -> bool;
		auto acquireNextImage(VulkanContext* ctx, VkSemaphore imageAvailableSemaphore, uint32_t timeout_ns, VkFence signalFence, uint32_t* outIndex) -> bool;
		auto present(VulkanContext* ctx, VkSemaphore renderCompleteSemaphore);

	public:
		VkSurfaceFormatKHR imageFormat;
		VkSwapchainKHR handle = VK_NULL_HANDLE;
		std::vector<SwapchainImage> images;

		uint32_t currentImageIndex;
		uint32_t maxFramesInFlight;

	public: // function pointers
		PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
		PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR; 
		PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
		PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
		PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
		PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
		PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
		PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
		PFN_vkQueuePresentKHR fpQueuePresentKHR;
	};
}

#endif // MXC_SWAPCHAIN_H
