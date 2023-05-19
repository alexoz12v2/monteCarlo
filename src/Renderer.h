#ifndef MXC_RENDERER_H
#define MXC_RENDERER_H

#include <vulkan/vulkan.h>

#include "VulkanCommon.h"
#include "Device.h"
#include "Swapchain.h"
#include "CommandBuffer.h"

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
		auto createDepthImages() -> bool;
        auto destroyDepthImages() -> void;

        // TODO encapsulate renderpass in VulkanContext
        auto createRenderPass() -> bool;
        auto destroyRenderPass() -> void;
        
        // TODO maybe: more flexible
        auto createPresentFramebuffers() -> bool;
        auto destroyPresentFramebuffers() -> void;
	};

	// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkResult.html, only some
	auto VulkanResultToString(VkResult) -> char const*;
}

#endif // MXC_RENDERER_H
