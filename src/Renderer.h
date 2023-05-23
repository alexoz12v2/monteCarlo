#ifndef MXC_RENDERER_H
#define MXC_RENDERER_H

#include <vulkan/vulkan.h>

#include "VulkanCommon.h"
#include "Device.h"
#include "Swapchain.h"
#include "Pipeline.h"
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
        static uint32_t constexpr OUT_ATTACHMENT_COUNT = 2;
	public:
		auto init(RendererConfig const&) -> bool;
		auto cleanup() -> void;

        // TODO cleanup
        auto getContextPointer() -> VulkanContext* { return &m_ctx; }
        auto getRenderPass() -> VkRenderPass { return m_ctx.renderPass; }

        template <typename F> requires std::is_invocable_r<VkResult, F, VkCommandBuffer>::value
        auto recordGraphicsCommands(F&& func) -> bool;

        auto submitFrame() -> void;

        auto resetCommandBuffers() -> void;
        auto resetCommandBuffersForDestruction() -> void; //difference from previous is that this calls vkDeviceWaitIdle()

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
        auto createDepthImages(DepthFormatProperties_t const depthFormatProperties) -> bool;
        auto destroyDepthImages() -> void;

        // TODO encapsulate renderpass in VulkanContext
        auto createRenderPass() -> bool;
        auto destroyRenderPass() -> void;
        
        // TODO maybe: more flexible
        auto createPresentFramebuffers() -> bool;
        auto destroyPresentFramebuffers() -> void;
	};

    // records commands to the next command buffer. NOT all
    template <typename F> requires std::is_invocable_r<VkResult, F, VkCommandBuffer>::value
    auto Renderer::recordGraphicsCommands(F&& func) -> bool
    {
        VK_CHECK(vkWaitForFences(m_ctx.device.logical, 1, &m_ctx.syncObjs[m_ctx.currentFramebufferIndex].renderCompleteFence, VK_TRUE, UINT64_MAX));
        VK_CHECK(vkResetFences(m_ctx.device.logical, 1, &m_ctx.syncObjs[m_ctx.currentFramebufferIndex].renderCompleteFence));

        m_ctx.swapchain.acquireNextImage(&m_ctx, m_ctx.syncObjs[m_ctx.currentFramebufferIndex].presentCompleteSemaphore, UINT32_MAX, VK_NULL_HANDLE,
                                         &m_ctx.currentFramebufferIndex);
        uint32_t i = m_ctx.currentFramebufferIndex;
        if (m_ctx.commandBuffers[i].isPending())
            m_ctx.commandBuffers[i].signalCompletion();
        m_ctx.commandBuffers[i].reset();

        VkClearValue clearValues[OUT_ATTACHMENT_COUNT] {};
        clearValues[0].color = {{.3f, .1f, .1f}};
        clearValues[1].depthStencil = { .depth = 1.f, .stencil = 0u };
        VkRenderPassBeginInfo renderPassBegin {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = nullptr,
            .renderPass = m_ctx.renderPass,
            .framebuffer = VK_NULL_HANDLE,
            .renderArea = {
                .offset = {.x = 0, .y = 0},
                .extent = {.width = m_ctx.framebufferWidth, .height = m_ctx.framebufferHeight}
            },
            .clearValueCount = OUT_ATTACHMENT_COUNT,
            .pClearValues = clearValues
        }; 
        static VkSubpassBeginInfo constexpr subpassBegin {
            .sType = VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO,
            .pNext = nullptr,
            .contents = VK_SUBPASS_CONTENTS_INLINE
        };
        static VkSubpassEndInfo constexpr subpassEnd {
            .sType = VK_STRUCTURE_TYPE_SUBPASS_END_INFO,
            .pNext = nullptr
        };
        // MXC_ASSERT(m_ctx.commandBuffers.size() == m_ctx.presentFramebuffers.size(), "assuming framebuffer number = graphics command buffer number");
        // begin command buffer
        m_ctx.commandBuffers[i].begin();
        
        // begin renderpass
        renderPassBegin.framebuffer = m_ctx.presentFramebuffers[i];
        vkCmdBeginRenderPass2(m_ctx.commandBuffers[i].handle, &renderPassBegin, &subpassBegin);

        VkResult res = func(m_ctx.commandBuffers[i].handle);

        vkCmdEndRenderPass2(m_ctx.commandBuffers[i].handle, &subpassEnd);
        m_ctx.commandBuffers[i].end();
        
        if (res != VK_SUCCESS)
            return false;

        return true;
    }

} // namespace mxc

#endif // MXC_RENDERER_H
