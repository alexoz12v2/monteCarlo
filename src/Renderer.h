#ifndef MXC_RENDERER_H
#define MXC_RENDERER_H

#include <vulkan/vulkan.h>

#include "VulkanCommon.h"
#include "Device.h"
#include "Swapchain.h"

#include "VulkanContext.inl"

namespace mxc
{
    struct RendererConfig
    {
        char const* platformSurfaceExtensionName;
        uint32_t windowWidth;
        uint32_t windowHeight;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
        void* platformHandle; 
        void* platformWindow;
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        wl_display* display; 
        wl_surface* window;
#elif defined(VK_USE_PLATFORM_XCB_KHR)
        xcb_connection_t* connection;
        xcb_window_t window;
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
        void* view;
#endif
    };


	struct VulkanContext;

	class Renderer
	{
	public:
		auto init(RendererConfig const&) -> bool;
		auto cleanup() -> void;

	private:
		VulkanContext m_ctx;

	private: // function pointers
#if defined(_DEBUG)
        PFN_vkSetDebugUtilsObjectNameEXT m_pfnSetDebugUtilsObjectNameEXT;
        PFN_vkSetDebugUtilsObjectTagEXT m_pfnSetDebugUtilsObjectTagEXT;
        PFN_vkCmdBeginDebugUtilsLabelEXT m_pfnCmdBeginDebugUtilsLabelEXT;
        PFN_vkCmdEndDebugUtilsLabelEXT m_pfnCmdEndDebugUtilsLabelEXT;
#endif
	private:
		// auto createDepthImages() -> bool;
	};

	// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkResult.html, only some
	auto VulkanResultToString(VkResult) -> char const*;
}

#endif // MXC_RENDERER_H
        // Depth resources
        // if (!vulkan_device_detect_depth_format(&context->device)) {
        //     context->device.depth_format = VK_FORMAT_UNDEFINED;
        //     KFATAL("Failed to find a supported format!");
        // }

        // if (!swapchain->depth_textures) {
        //     swapchain->depth_textures = (texture*)kallocate(sizeof(texture) * swapchain->image_count, MEMORY_TAG_RENDERER);
        // }

        // for (u32 i = 0; i < context->swapchain.image_count; ++i) {
        //     // Create depth image and its view.
        //     char formatted_name[TEXTURE_NAME_MAX_LENGTH] = {0};
        //     string_format(formatted_name, "swapchain_image_%u", i);

        //     vulkan_image* image = kallocate(sizeof(vulkan_image), MEMORY_TAG_TEXTURE);
        //     vulkan_image_create(
        //         context,
        //         TEXTURE_TYPE_2D,
        //         swapchain_extent.width,
        //         swapchain_extent.height,
        //         context->device.depth_format,
        //         VK_IMAGE_TILING_OPTIMAL,
        //         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        //         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        //         true,
        //         VK_IMAGE_ASPECT_DEPTH_BIT,
        //         formatted_name,
        //         image);

        //     // Wrap it in a texture.
        //     texture_system_wrap_internal(
        //         "__kohi_default_depth_texture__",
        //         swapchain_extent.width,
        //         swapchain_extent.height,
        //         context->device.depth_channel_count,
        //         false,
        //         true,
        //         false,
        //         image,
        //         &context->swapchain.depth_textures[i]);
        // }
