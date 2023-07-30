#include "Renderer.h"
#include "VulkanCommon.h"

#define MXC_VULKAN_CONTEXT_INL_IMPLEMENTATION
#include <VulkanContext.inl>

#include "logging.h"

#include <cstdlib>
#include <cstring>
#include <vector>
#include <array>

namespace mxc 
{
    auto createCommandBuffers(VulkanContext* ctx) -> void;
    constexpr auto renderPassCreateInfo2() -> VkRenderPassCreateInfo2;
    constexpr auto attachmentDescription2() -> VkAttachmentDescription2;
    constexpr auto subpassDependency2() -> VkSubpassDependency2;
    constexpr auto attachmentReference2() -> VkAttachmentReference2;
    constexpr auto subpassDescription2() -> VkSubpassDescription2;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        [[maybe_unused]] void* pUserData)
    {
        char const* msgTypeStr;
        switch (messageType)
        {
            case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: msgTypeStr = "VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT"; break;
            case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: msgTypeStr = "VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT"; break;
            case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: msgTypeStr = "VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT"; break;
        }
        
        switch (messageSeverity)
        {
	    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: 
                MXC_TRACE("Vulkan Validation of type %s: %s", msgTypeStr, pCallbackData->pMessage);
                break;
	    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: 
                MXC_INFO("Vulkan Validation of type %s: %s", msgTypeStr, pCallbackData->pMessage);
                break;
	    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: 
                MXC_WARN("Vulkan Validation of type %s: %s", msgTypeStr, pCallbackData->pMessage);
                break;
	    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: 
                MXC_ERROR("Vulkan Validation of type %s: %s", msgTypeStr, pCallbackData->pMessage);
                break;
        }

        return VK_FALSE;
    }

    auto Renderer::init(RendererConfig const& config) -> bool
    {
        MXC_INFO("Initializing the Renderer...");
        // Setup Vulkan instance.
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.apiVersion = VK_API_VERSION_1_3;
        appInfo.pApplicationName = "monte carlo";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "monte carlo";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Obtain a list of required extensions
        std::vector<char const*> requiredExtensions;
        requiredExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME); // VK_KHR_surface
        requiredExtensions.push_back(config.platformSurfaceExtensionName); // Generic surface extension
#if defined(_DEBUG)
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        MXC_DEBUG("Required extensions:");
        for (uint32_t i = 0; i < requiredExtensions.size(); ++i) {
            MXC_DEBUG(requiredExtensions[i]);
        }
#endif

        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();

        uint32_t availableExtension_count = 0;
        vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &availableExtension_count, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(availableExtension_count);
        vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &availableExtension_count, availableExtensions.data());

        // Verify required extensions are available.
        for (uint32_t i = 0; i < requiredExtensions.size(); ++i) 
        {
            bool found = false;
            for (uint32_t j = 0; j < availableExtension_count; ++j) 
            {
                if (0 == strcmp(requiredExtensions[i], availableExtensions[j].extensionName)) 
                {
                    found = true;
                    MXC_INFO("Required exension found: %s...", requiredExtensions[i]);
                    break;
                }
            }

            if (!found) 
            {
                MXC_ERROR("Required extension is missing: %s", requiredExtensions[i]);
                return false;
            }
        }

        // Validation layers.
        std::vector<const char*> requiredValidationLayerNames;

        // If validation should be done, get a list of the required validation layert names
        // and make sure they exist. Validation layers should only be enabled on non-release builds.
#if defined(_DEBUG)
        MXC_INFO("Validation layers enabled. Enumerating...");

        // The list of validation layers required.
        requiredValidationLayerNames.push_back("VK_LAYER_KHRONOS_validation");

        // NOTE: enable this when needed for debugging.
        // darray_push(required_validation_layer_names, &"VK_LAYER_LUNARG_api_dump");

        // Obtain a list of available validation layers
        uint32_t availableLayer_count = 0;
        VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayer_count, nullptr));
        std::vector<VkLayerProperties> availableLayers(availableLayer_count);
        VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayer_count, availableLayers.data()));

        MXC_INFO("starting to check for layers");
        // Verify all required layers are available.
        for (uint32_t i = 0; i < requiredValidationLayerNames.size(); ++i) 
        {
            bool found = false;
            for (uint32_t j = 0; j < availableLayer_count; ++j) 
            {
                if (0 == strcmp(requiredValidationLayerNames[i], availableLayers[j].layerName)) 
                {
                    found = true;
                    MXC_INFO("Found validation layer: %s...", requiredValidationLayerNames[i]);
                    break;
                }
            }

            if (!found) 
            {
                MXC_ERROR("Required validation layer is missing: %s", requiredValidationLayerNames[i]);
                return false;
            }
        }

        MXC_INFO("All required validation layers are present.");
#endif

        createInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayerNames.size());
        createInfo.ppEnabledLayerNames = requiredValidationLayerNames.data();

        VkResult instanceResult = vkCreateInstance(&createInfo, nullptr, &m_ctx.instance);
        if (instanceResult != VK_SUCCESS) {
            const char* resultString = vulkanResultToString(instanceResult);
            MXC_ERROR("Vulkan instance creation failed with result: '%s'", resultString);
            return false;
        }

        MXC_INFO("Vulkan Instance created.");

        // Debugger
#if defined(_DEBUG)
        MXC_DEBUG("Creating Vulkan debugger...");
        uint32_t log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = log_severity;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;

        PFN_vkCreateDebugUtilsMessengerEXT func =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_ctx.instance, "vkCreateDebugUtilsMessengerEXT");
        assert(func && "Failed to create debug messenger!");
        VK_CHECK(func(m_ctx.instance, &debugCreateInfo, nullptr, &m_ctx.debugMessenger));
        MXC_DEBUG("Vulkan debugger created.");

        // Load up debug function pointers.
        m_pfnSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(m_ctx.instance, "vkSetDebugUtilsObjectNameEXT");
        if (!m_pfnSetDebugUtilsObjectNameEXT) {
            MXC_WARN("Unable to load function pointer for vkSetDebugUtilsObjectNameEXT. Debug functions associated with this will not work.");
        }
        m_pfnSetDebugUtilsObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT)vkGetInstanceProcAddr(m_ctx.instance, "vkSetDebugUtilsObjectTagEXT");
        if (!m_pfnSetDebugUtilsObjectTagEXT) {
            MXC_WARN("Unable to load function pointer for vkSetDebugUtilsObjectTagEXT. Debug functions associated with this will not work.");
        }

        m_pfnCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(m_ctx.instance, "vkCmdBeginDebugUtilsLabelEXT");
        if (!m_pfnCmdBeginDebugUtilsLabelEXT) {
            MXC_WARN("Unable to load function pointer for vkCmdBeginDebugUtilsLabelEXT. Debug functions associated with this will not work.");
        }

        m_pfnCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(m_ctx.instance, "vkCmdEndDebugUtilsLabelEXT");
        if (!m_pfnCmdEndDebugUtilsLabelEXT) {
            MXC_WARN("Unable to load function pointer for vkCmdEndDebugUtilsLabelEXT. Debug functions associated with this will not work.");
        }
#endif

        // Surface
        MXC_INFO("Creating Vulkan surface...");
        if (!
#if defined(VK_USE_PLATFORM_WIN32_KHR)
            m_ctx.swapchain.initSurface(&m_ctx, config.platformHandle, config.platformWindow)
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
            m_ctx.swapchain.initSurface(&m_ctx, config.display, config.window)
#elif defined(VK_USE_PLATFORM_XCB_KHR)
            m_ctx.swapchain.initSurface(&m_ctx, config.connection, config.window)
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
            m_ctx.swapchain.initSurface(&m_ctx, config.view)
#else
    #error "shouldn't be here"
#endif 
        ) {
            MXC_ERROR("Failed to create platform surface!");
            return false;
        }
        MXC_INFO("Vulkan surface created.");

        // initialize swapchain extension function pointers for instance level functions
        m_ctx.swapchain.initInstanceFunctionPointers(&m_ctx);

        // Device creation
        PhysicalDeviceRequirements requirements;
        requirements.extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        requirements.extensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
        MXC_INFO("required device extensions:");
        for (uint32_t i = 0; i != requirements.extensions.size(); ++i)
        {
            MXC_INFO("\t%s", requirements.extensions[i]);
        }

        if (!m_ctx.device.create(&m_ctx, requirements))
        {
            MXC_ERROR("Failed to create device!");
            return false;
        }

        // Retrieve the function pointer
        fpCmdPushDescriptorSetWithTemplateKHR = reinterpret_cast<PFN_vkCmdPushDescriptorSetWithTemplateKHR>(
            vkGetInstanceProcAddr(m_ctx.instance, "vkCmdPushDescriptorSetWithTemplateKHR")
        );

        if (!fpCmdPushDescriptorSetWithTemplateKHR) {
            MXC_ERROR("failed to find vkCmdPushDescriptorSetWithTemplateKHR");
            return false;
        }

        // Swapchain
        m_ctx.swapchain.initDeviceFunctionPointers(&m_ctx);
        m_ctx.framebufferWidth = config.windowWidth;
        m_ctx.framebufferHeight = config.windowHeight;
        m_ctx.swapchain.create(&m_ctx, m_ctx.framebufferWidth, m_ctx.framebufferHeight, /*vsync*/true);

        // Query for depth format and create depth image TODO: do it and make it configurable
        DepthFormatProperties_t formatProperties;
        MXC_ASSERT(m_ctx.device.selectDepthFormat(&m_ctx.depthFormat, &formatProperties), "no supported depth image format found");

        // Create command buffers. TODO configurable
        createCommandBuffers(&m_ctx);

        // Create sync objects.
        createSynchronizationPrimitives();
        MXC_DEBUG("Created Vulkan Synchronization primitives");
        
        // Create Depth Images (logging is in there)
        if (!createDepthImages(formatProperties))
        {
            MXC_ERROR("failed to create depth images");
            return false;
        }
        MXC_DEBUG("Created Depth Images");

        // create renderpass
        if (!createRenderPass())
        {
            MXC_ERROR("failed to create renderPass");
            return false;
        }
        MXC_DEBUG("Created default renderPass");

        // create framebuffers
        if (!createPresentFramebuffers())
        {
            MXC_ERROR("failed to create present framebuffers!");
        }
        MXC_DEBUG("creates present framebuffers");
        
        m_prepared = true;

        MXC_INFO("Vulkan renderer initialized successfully.");
        return true;
    }

    auto Renderer::cleanup() -> void
    {
        m_prepared = false;
        vkDeviceWaitIdle(m_ctx.device.logical);
//
//        // Destroy in the opposite order of creation.
//        // Destroy buffers
//        renderer_renderbuffer_destroy(&context->object_vertex_buffer);
//        renderer_renderbuffer_destroy(&context->object_index_buffer);
//
        // TODO move elsewhere: Renderpass and framebuffers
        MXC_DEBUG("Destroying Framebuffers and RenderPass...");
        destroyPresentFramebuffers();
        destroyRenderPass();

        // Destroy Depth Images
        MXC_DEBUG("Destroying Depth images");
        destroyDepthImages();

        // Sync objects
        MXC_DEBUG("Destroying Vulkan Synchronization Primitives...");
        destroySynchronizationPrimitives();

        // Command buffers
        MXC_DEBUG("Freeing %zu command buffers...", m_ctx.commandBuffers.size());
        CommandBuffer::freeMany(&m_ctx, m_ctx.commandBuffers.data(), static_cast<uint32_t>(m_ctx.commandBuffers.size()));
        m_ctx.computeCommandBuffer.free(&m_ctx);

        // Swapchain
        MXC_DEBUG("Destroying the Swapchain...");
        m_ctx.swapchain.destroy(&m_ctx);

        MXC_DEBUG("Destroying Vulkan device...");
        m_ctx.device.destroy(&m_ctx);

        MXC_DEBUG("Destroying Vulkan surface...");
        if (m_ctx.surface) 
        {
            vkDestroySurfaceKHR(m_ctx.instance, m_ctx.surface, nullptr);
            m_ctx.surface = VK_NULL_HANDLE;
        }

#if defined(_DEBUG)
        MXC_DEBUG("Destroying Vulkan debugger...");
        if (m_ctx.debugMessenger) 
        {
            PFN_vkDestroyDebugUtilsMessengerEXT func =
                (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_ctx.instance, "vkDestroyDebugUtilsMessengerEXT");
            func(m_ctx.instance, m_ctx.debugMessenger, nullptr);
        }
#endif

        MXC_DEBUG("Destroying Vulkan instance...");
        vkDestroyInstance(m_ctx.instance, nullptr);
    }

    // https://www.reddit.com/r/vulkan/comments/s80reu/subpass_dependencies_what_are_those_and_why_do_i/
    auto Renderer::createRenderPass() -> bool
    {
        // ----------- Define Attachment Descriptions------------------------------
        std::array attachmentDescriptions {
            attachmentDescription2(), // color attachment
            attachmentDescription2() // depth attachment
        };
        attachmentDescriptions[0].format = m_ctx.swapchain.imageFormat.format;
        attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        attachmentDescriptions[1].format = m_ctx.depthFormat;
	attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        // The source scope refers to the pipeline stages or memory access operations that occur before a synchronization command, while the 
        // destination scope refers to the pipeline stages or memory access operations that occur after the synchronization command.
        // --------------- Define Subpass Dependencies ----------------------------
        std::array subpassDependencies {
            subpassDependency2(), // color attachment
            subpassDependency2()  // depth attachment
        };
        // The source subpass is the subpass that generates the data or resource that is being used by the destination subpass. The destination subpass is the subpass that consumes the data or resource       
        // dependency between external usage (presentation) and the subpass
        // color attachment layout transition synchronization
        subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL; // dependency is relative to the operations that happen after (logically) the specified stages in the the external subpass(presentation) (before the synchronization command)
        subpassDependencies[0].dstSubpass = 0; // ... and before (logically) the first subpass (the only one) in the specified stages. This means that this synchronization operation happens before the renderpass begins
        subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // transition can happen after color output stage of the srcSubpass
        subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // transition can happen before color output stage of the dstSubpass
        subpassDependencies[0].srcAccessMask = 0; // memory access cannot happen after the srcStageMask of the srcSubpass
        subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT; // memory access that chan happen in dstStageMask of dstSubpass, after the synchronization command

        // depth attachment
        subpassDependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependencies[1].dstSubpass = 0;
        subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        subpassDependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        subpassDependencies[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

        // --------------- Define subpass descriptions ----------------------------
        std::array attachmentReferences {
            attachmentReference2(),
            attachmentReference2()
        };

        attachmentReferences[0].attachment = 0;
        attachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachmentReferences[0].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        attachmentReferences[1].attachment = 1;
        attachmentReferences[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachmentReferences[1].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        VkSubpassDescription2 subpassDescription = subpassDescription2();
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &attachmentReferences[0];
        subpassDescription.pDepthStencilAttachment = &attachmentReferences[1];

        // ---------------- Create RenderPass--------------------------------------
        VkRenderPassCreateInfo2 renderPassCI = renderPassCreateInfo2();
        renderPassCI.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
        renderPassCI.pAttachments = attachmentDescriptions.data();
        renderPassCI.subpassCount = 1;
        renderPassCI.pSubpasses = &subpassDescription;
        renderPassCI.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
        renderPassCI.pDependencies = subpassDependencies.data();

        VK_CHECK(vkCreateRenderPass2(m_ctx.device.logical, &renderPassCI, nullptr, &m_ctx.renderPass));
        MXC_INFO("Created a RenderPass"); // TODO add information
        return true;
    }

    auto Renderer::destroyRenderPass() -> void
    {
        vkDestroyRenderPass(m_ctx.device.logical, m_ctx.renderPass, nullptr);
        m_ctx.renderPass = VK_NULL_HANDLE;
        MXC_DEBUG("Destroyed RenderPass");
    }

    auto Renderer::createPresentFramebuffers() -> bool
    {
        static uint32_t constexpr ATTACHMENT_COUNT = 2; // TODO configurable
        m_ctx.presentFramebuffers.resize(m_ctx.swapchain.images.size());

        VkFramebufferCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr, // imageless
            .flags = 0,
            .renderPass = m_ctx.renderPass,
            .attachmentCount = ATTACHMENT_COUNT,
            .pAttachments = nullptr,
            .width = m_ctx.framebufferWidth,
            .height = m_ctx.framebufferHeight,
            .layers = 1,
        };

        for (uint8_t i = 0; i != m_ctx.swapchain.images.size(); ++i)
        {
            std::array<VkImageView, ATTACHMENT_COUNT> attachments{m_ctx.swapchain.images[i].view, m_ctx.depthImages[i].view.handle};
            createInfo.pAttachments = attachments.data();
            VK_CHECK(vkCreateFramebuffer(m_ctx.device.logical, &createInfo, nullptr, &m_ctx.presentFramebuffers[i]));
        }
        return true;
    }

    [[nodiscard]] auto Renderer::submitFrame() -> RendererStatus
    {
        if (!m_ctx.commandBuffers[m_ctx.currentFramebufferIndex].isExecutable()) m_ctx.commandBuffers[m_ctx.currentFramebufferIndex].signalCompletion();
        MXC_ASSERT(m_ctx.commandBuffers[m_ctx.currentFramebufferIndex].isExecutable(), "command buffer is not executable");
        
        VkSemaphoreSubmitInfo waitSemaphoreInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = m_ctx.syncObjs[m_ctx.currentFramebufferIndex].presentCompleteSemaphore,
            .value = 0, // ignored for binary semaphores
            .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .deviceIndex = 0, // index of a device within a group
        };
        VkCommandBufferSubmitInfo commandBufferInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext = nullptr,
            .commandBuffer = m_ctx.commandBuffers[m_ctx.currentFramebufferIndex].handle,
            .deviceMask = 0
        };
        VkSemaphoreSubmitInfo signalSemaphoreInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = m_ctx.syncObjs[m_ctx.currentFramebufferIndex].renderCompleteSemaphore,
            .value = 0, // ignored for binary semaphores
            .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            .deviceIndex = 0, // index of a device within a group
        };
        VkSubmitInfo2 submitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .pNext = nullptr,
            .flags = 0,
            .waitSemaphoreInfoCount = 1, // presentCompleteSemaphores[m_ctx.currentFramebufferIndex]
            .pWaitSemaphoreInfos = &waitSemaphoreInfo,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &commandBufferInfo,
            .signalSemaphoreInfoCount = 1, // renderCompleteSemaphores[m_ctx.currentFramebufferIndex]
            .pSignalSemaphoreInfos = &signalSemaphoreInfo
        };

        VK_CHECK(vkQueueSubmit2(m_ctx.device.graphicsQueue, 1, &submitInfo, m_ctx.syncObjs[m_ctx.currentFramebufferIndex].renderCompleteFence));
        m_ctx.commandBuffers[m_ctx.currentFramebufferIndex].signalSubmit();
        
        if (m_prepared)
        {
            // swapchain checks for resize and calls it if necessary
            SwapchainStatus status = m_ctx.swapchain.present(&m_ctx, m_ctx.syncObjs[m_ctx.currentFramebufferIndex].renderCompleteSemaphore);
            if (status == SwapchainStatus::WINDOW_RESIZED)
            {
                MXC_WARN("Window Resize on present");
                onResize();
                return RendererStatus::WINDOW_RESIZED;
            }
            if (status == SwapchainStatus::FATAL)
                return RendererStatus::FATAL;

            if (++m_ctx.currentFramebufferIndex >= m_ctx.presentFramebuffers.size())
                m_ctx.currentFramebufferIndex = 0;
        }
        
        m_prepared = true;

        return RendererStatus::OK;
    }

    [[nodiscard]] auto Renderer::submitCompute() -> RendererStatus
    {
        VkSemaphoreSubmitInfo waitSemaphoreInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = m_ctx.syncObjs[m_ctx.currentFramebufferIndex].presentCompleteSemaphore,
            .value = 0, // ignored for binary semaphores
            .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            .deviceIndex = 0,
        };
        VkCommandBufferSubmitInfo commandBufferInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext = nullptr,
            .commandBuffer = m_ctx.computeCommandBuffer.handle,
            .deviceMask = 0
        };
        VkSemaphoreSubmitInfo signalSemaphoreInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = m_ctx.computeSemaphore,
            .value = 0, // ignored for binary semaphores
            .stageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            .deviceIndex = 0,
        };
        VkSubmitInfo2 submitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .pNext = nullptr,
            .flags = 0,
            .waitSemaphoreInfoCount = 1, // presentCompleteSemaphores[m_ctx.currentFramebufferIndex]
            .pWaitSemaphoreInfos = &waitSemaphoreInfo,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &commandBufferInfo,
            .signalSemaphoreInfoCount = 1, // renderCompleteSemaphores[m_ctx.currentFramebufferIndex]
            .pSignalSemaphoreInfos = &signalSemaphoreInfo
        };

        VK_CHECK(vkQueueSubmit2(m_ctx.device.computeQueue, 1, &submitInfo, m_ctx.syncObjs[m_ctx.currentFramebufferIndex].renderCompleteFence));
        m_ctx.computeCommandBuffer.signalSubmit();
        vkQueueWaitIdle(m_ctx.device.computeQueue);
        MXC_WARN("11");

        return RendererStatus::OK;
    }

    [[nodiscard]] auto Renderer::transitionAndpresentFrame([[maybe_unused]] VkImageLayout inLayout) -> RendererStatus
    {
        uint32_t const i = m_ctx.currentFramebufferIndex;
        VkImage swapchainImage = m_ctx.swapchain.images[i].handle;
        CommandBuffer& cmdBuf = m_ctx.commandBuffers[i];

        cmdBuf.begin();
        MXC_ASSERT(cmdBuf.canRecord(), "Image memory barrier Command Buffer is not in recording state");


        // Image memory barrier to make sure that compute shader writes are finished before sampling from the texture
        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageMemoryBarrier.image = swapchainImage;
        imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	MXC_TRACE("Swapchain image %u released by graphics queue");
        if (m_ctx.device.queueFamilies.graphics != m_ctx.device.queueFamilies.compute)
        {
            // Acquire barrier for graphics queue
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            imageMemoryBarrier.srcQueueFamilyIndex = m_ctx.device.queueFamilies.compute;
            imageMemoryBarrier.dstQueueFamilyIndex = m_ctx.device.queueFamilies.graphics;
            vkCmdPipelineBarrier(
                cmdBuf.handle,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0,//VK_FLAGS_NONE,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);
	    MXC_DEBUG("Swapchain image %u acquired by graphics queue");
        }
        else
        {
            // Combined barrier on single queue family
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vkCmdPipelineBarrier(
                cmdBuf.handle,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                0,//VK_FLAGS_NONE,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);
        }

        m_ctx.device.insertImageMemoryBarrier(
            cmdBuf.handle, swapchainImage,
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 
            VK_IMAGE_ASPECT_COLOR_BIT);
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        if (m_ctx.device.queueFamilies.graphics != m_ctx.device.queueFamilies.compute)
        {
            // Release barrier from graphics queue
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            imageMemoryBarrier.dstAccessMask = 0;
            imageMemoryBarrier.srcQueueFamilyIndex = m_ctx.device.queueFamilies.graphics;
            imageMemoryBarrier.dstQueueFamilyIndex = m_ctx.device.queueFamilies.compute;
            vkCmdPipelineBarrier(
                cmdBuf.handle,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0,//VK_FLAGS_NONE,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);
	    MXC_TRACE("Swapchain image %u released by graphics queue");
        }

        MXC_ASSERT(cmdBuf.end(), "Couldn't end recording of image memory barrier Command Buffer");

        m_ctx.device.flushCommandBuffer(&cmdBuf, CommandType::GRAPHICS);

        RendererStatus status = presentFrame();
        if (status == RendererStatus::OK)
        { 
            MXC_WARN("Post presentation");
            VkImage swapchainImage = m_ctx.swapchain.images[m_ctx.currentFramebufferIndex].handle;
            cmdBuf.begin();
            m_ctx.device.insertImageMemoryBarrier(
                cmdBuf.handle, swapchainImage,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_ASPECT_COLOR_BIT);

            if (m_ctx.device.queueFamilies.graphics != m_ctx.device.queueFamilies.compute)
            {
                VkImageMemoryBarrier imageMemoryBarrier{};
                imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageMemoryBarrier.image = swapchainImage;
                imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

                imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
                imageMemoryBarrier.dstAccessMask = 0;
                imageMemoryBarrier.srcQueueFamilyIndex = m_ctx.device.queueFamilies.graphics;
                imageMemoryBarrier.dstQueueFamilyIndex = m_ctx.device.queueFamilies.compute;
            }

            cmdBuf.end();
            m_ctx.device.flushCommandBuffer(&cmdBuf, CommandType::GRAPHICS);
            vkQueueWaitIdle(m_ctx.device.graphicsQueue);
            MXC_WARN("Post transition");
        }

        MXC_WARN("submission Done");
        return RendererStatus::OK;
    }

    [[nodiscard]] auto Renderer::presentFrame() -> RendererStatus
    {
        if (m_prepared)
        {
            SwapchainStatus status = m_ctx.swapchain.present(&m_ctx, m_ctx.computeSemaphore);// m_ctx.syncObjs[m_ctx.currentFramebufferIndex].renderCompleteSemaphore);
            if (status == SwapchainStatus::WINDOW_RESIZED)
            {
                MXC_WARN("Window Resize on present");
                onResize();
                return RendererStatus::WINDOW_RESIZED;
            }
            if (status == SwapchainStatus::FATAL)
                return RendererStatus::FATAL;

            if (++m_ctx.currentFramebufferIndex >= m_ctx.presentFramebuffers.size())
                m_ctx.currentFramebufferIndex = 0;
        }
        return RendererStatus::OK;
    }
    
    auto Renderer::destroyPresentFramebuffers() -> void
    {
        for (uint8_t i = 0; i != m_ctx.presentFramebuffers.size(); ++i)
        {   
            vkDestroyFramebuffer(m_ctx.device.logical, m_ctx.presentFramebuffers[i], nullptr);
        }
    }

    auto Renderer::createDepthImages(DepthFormatProperties_t const depthFormatProperties) -> bool
    {
        VkImageAspectFlags stencil = (depthFormatProperties & DepthFormatProperties_v::SUPPORTS_STENCIL) == DepthFormatProperties_v::SUPPORTS_STENCIL 
                                    ? VK_IMAGE_ASPECT_STENCIL_BIT 
                                    : VK_IMAGE_ASPECT_DEPTH_BIT;
        
        VkFormat depthFormat = VK_FORMAT_UNDEFINED;
        if (!m_ctx.device.selectDepthFormat(&depthFormat, nullptr))
        {
            MXC_ERROR("Couldn't find suitable depth format");
            return false;
        }
            
        m_ctx.depthImages.reserve(m_ctx.swapchain.images.size());
        for (uint32_t i = 0; i != m_ctx.swapchain.images.size(); ++i)
        {
            // create depth images and views. TODO I can use emplace back if I write a custom constructor for the ImageViewPair
            Image depthImage(
                VK_IMAGE_TYPE_2D, 
                {.width = m_ctx.framebufferWidth, .height = m_ctx.framebufferHeight, .depth = 1}, 
                depthFormat, 
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
            ImageView depthView(
                VK_IMAGE_VIEW_TYPE_2D,
                VK_IMAGE_ASPECT_DEPTH_BIT | stencil);
            VkImageLayout targetLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            // Note: you can hard code tiling because in selectDepthFormat tiling is hardcoded
            m_ctx.device.createImage(&m_ctx, VK_IMAGE_TILING_OPTIMAL, &depthImage, &targetLayout, &depthView);
            m_ctx.depthImages.push_back({depthImage, depthView});
            MXC_ASSERT(depthImage.handle != VK_NULL_HANDLE && depthView.handle != VK_NULL_HANDLE, "after creating depth image and view %u null", i);
            MXC_TRACE("created depth image %p and view %p", depthImage.handle, depthView.handle);
        }

        MXC_DEBUG("Vulkan depth images created");
        return true;
    }

    auto Renderer::destroyDepthImages() -> void
    {
        for (uint32_t i = 0; i != m_ctx.depthImages.size(); ++i)
        {   
            m_ctx.device.destroyImageView(&m_ctx.depthImages[i].view);
            m_ctx.device.destroyImage(&m_ctx.depthImages[i].image);
        }
        m_ctx.depthImages.clear();
        MXC_DEBUG("Vulkan depth images destroyed");
    }

    auto Renderer::resetComputeCommandBuffer() -> void
    {   
        m_ctx.computeCommandBuffer.reset();
    }

    auto Renderer::resetGraphicsCommandBuffers() -> void
    {
        for (uint32_t i = 0; i != m_ctx.commandBuffers.size(); ++i)
        {
            m_ctx.commandBuffers[i].reset();
        }
    }

    auto Renderer::resetCommandBuffersForDestruction() -> void
    {
        VK_CHECK(vkDeviceWaitIdle(m_ctx.device.logical));
        for (uint32_t i = 0; i != m_ctx.commandBuffers.size(); ++i)
        {
            if (m_ctx.commandBuffers[i].isPending())
                m_ctx.commandBuffers[i].signalCompletion();
            m_ctx.commandBuffers[i].reset();
        }

        if (m_ctx.computeCommandBuffer.isPending())
            m_ctx.computeCommandBuffer.signalCompletion();
        m_ctx.computeCommandBuffer.reset();
    }

    auto Renderer::resizeFramebufferSizes(uint32_t newWidth, uint32_t newHeight) -> void
    {
        m_ctx.framebufferWidth = newWidth;
        m_ctx.framebufferHeight = newHeight;
    }

    auto Renderer::getFramebufferSizes() -> VkExtent2D
    {
        VkExtent2D size = fpGetSurfaceFramebufferSize ? fpGetSurfaceFramebufferSize() : VkExtent2D{m_ctx.framebufferWidth, m_ctx.framebufferHeight};
        return size;
    }

    auto Renderer::onResize() -> void
    {
        m_prepared = false;
        //bool upToDate = false;

        //while (!upToDate)
        //{
        auto [newWidth, newHeight] = getFramebufferSizes();
        MXC_ASSERT(newWidth == getFramebufferSizes().width && newHeight == getFramebufferSizes().height, 
               "framebufferWidth = %u, framebufferHeight = %u\nwidth = %u, height = %u\n", 
               newWidth, newHeight, getFramebufferSizes().width, getFramebufferSizes().height);

        // wait previous pending command buffers on the device 
        vkDeviceWaitIdle(m_ctx.device.logical);
        MXC_DEBUG("Resizing the Renderer...");

        // recreate swapchain
        MXC_DEBUG("Recreating the swapchain with extent %u x %u...", newWidth, newHeight);
        m_ctx.swapchain.create(&m_ctx, newWidth, newHeight);
        
        MXC_DEBUG("Recreated the swapchain");

        // recreate framebuffers
        m_ctx.framebufferWidth = newWidth; 
        m_ctx.framebufferHeight = newHeight;

        MXC_DEBUG("Recreating depth images...");
        destroyDepthImages();
        createDepthImages(DepthFormatProperties_v::SUPPORTS_STENCIL);
        MXC_DEBUG("Recreated depth images");

        MXC_DEBUG("Recreating present framebuffers...");
        destroyPresentFramebuffers();
        createPresentFramebuffers();
        MXC_DEBUG("recreated present framebuffers");

        // recreate command buffers, because they may store references to the destroyed framebuffers
        // specified as a parameter in vkCmdBeginRenderPass2
        // TODO now they are hardcoded to be graphics command buffers
        MXC_DEBUG("Recreating command buffers...");
        resetCommandBuffersForDestruction();
        CommandBuffer::freeMany(&m_ctx, m_ctx.commandBuffers.data(), static_cast<uint32_t>(m_ctx.commandBuffers.size()));
        m_ctx.computeCommandBuffer.free(&m_ctx);
        createCommandBuffers(&m_ctx);
        MXC_DEBUG("Recreated command buffers");

        MXC_DEBUG("Recreating synchronization primitives...");
        destroySynchronizationPrimitives();
        createSynchronizationPrimitives();
        MXC_DEBUG("Recreated synchronization primitives");

        // If renderpass becomes incompatible (i.e. attachments of framebuffer) change, we need to recreate
        // renderpass and graphics pipeline as well. Won't handle that now TODO?

        vkDeviceWaitIdle(m_ctx.device.logical);
        //   upToDate = newWidth == getFramebufferSizes().width && newHeight == getFramebufferSizes().height;
        //}

        MXC_DEBUG("Resized the Renderer");
        m_prepared = true;
    }
    
    auto Renderer::createSynchronizationPrimitives() -> bool
    {
        m_ctx.syncObjs.reserve(m_ctx.swapchain.images.size());

        VkSemaphoreTypeCreateInfo const semaphoreTypeCI {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .pNext = nullptr,
            .semaphoreType = VK_SEMAPHORE_TYPE_BINARY,
            .initialValue = 0 // ignored if binary
        };
        VkSemaphoreCreateInfo const semaphoreCI {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = reinterpret_cast<void const*>(&semaphoreTypeCI),
            .flags = 0
        };
        VkFenceCreateInfo const fenceCI {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };
        for (uint8_t i = 0; i != m_ctx.swapchain.images.size(); ++i)
        {
            VkSemaphore semaphore[2];
            VK_CHECK(vkCreateSemaphore(m_ctx.device.logical, &semaphoreCI, nullptr, &semaphore[0]));
            VK_CHECK(vkCreateSemaphore(m_ctx.device.logical, &semaphoreCI, nullptr, &semaphore[1]));

            VkFence fence;
            VK_CHECK(vkCreateFence(m_ctx.device.logical, &fenceCI, nullptr, &fence));
            FrameObjects f = {m_ctx.commandBuffers[i].handle, fence, semaphore[0], semaphore[1]};
            // m_ctx.syncObjs.emplace_back(m_ctx.commandBuffers[i].handle, fence, semaphore[0], semaphore[1]); substitution failure?
            m_ctx.syncObjs.push_back(f); // TODO cmdBuffers configurable
        }

        VK_CHECK(vkCreateSemaphore(m_ctx.device.logical, &semaphoreCI, nullptr, &m_ctx.computeSemaphore));

        return true;
    }

    auto Renderer::destroySynchronizationPrimitives() -> void
    {
        for (uint8_t i = 0; i != m_ctx.syncObjs.size(); ++i)
        {
            vkDestroySemaphore(m_ctx.device.logical, m_ctx.syncObjs[i].renderCompleteSemaphore, nullptr);
            vkDestroySemaphore(m_ctx.device.logical, m_ctx.syncObjs[i].presentCompleteSemaphore, nullptr);

            vkDestroyFence(m_ctx.device.logical, m_ctx.syncObjs[i].renderCompleteFence, nullptr);
        }

        vkDestroySemaphore(m_ctx.device.logical, m_ctx.computeSemaphore, nullptr);

        m_ctx.syncObjs.clear();
        MXC_DEBUG("Cleared synchronization objects. vector<SynchObjs> size = %zu", m_ctx.syncObjs.size());
    }

    auto createCommandBuffers(VulkanContext* ctx) -> void
    {
        uint32_t const count = static_cast<uint32_t>(ctx->swapchain.images.size());
        ctx->commandBuffers.resize(count);
        CommandBuffer::allocateMany(ctx, CommandType::GRAPHICS, ctx->commandBuffers.data(), count);
        ctx->computeCommandBuffer.allocate(ctx, CommandType::COMPUTE);
        MXC_DEBUG("Vulkan Command Buffers Allocated");
    }

    constexpr auto renderPassCreateInfo2() -> VkRenderPassCreateInfo2
    {
        return {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
            .pNext = nullptr,
            .flags = 0,
            .attachmentCount = 0,
            .pAttachments = nullptr,
            .subpassCount = 0,
            .pSubpasses = nullptr,
            .dependencyCount = 0,
            .pDependencies = nullptr,
            .correlatedViewMaskCount = 0,
            .pCorrelatedViewMasks = nullptr
        };
    }

    constexpr auto attachmentDescription2() -> VkAttachmentDescription2
    {
        return {
            .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
            .pNext = nullptr,
            .flags = 0,
            .format = VK_FORMAT_UNDEFINED,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // expected layout
            .finalLayout = VK_IMAGE_LAYOUT_UNDEFINED, // transitioned layout (performed by render pass)
        };
    }

    constexpr auto subpassDependency2() -> VkSubpassDependency2
    {
        return {
            .sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
            .pNext = nullptr,
            .srcSubpass = UINT32_MAX,
            .dstSubpass = UINT32_MAX,
            .srcStageMask = 0,
            .dstStageMask = 0
        };
    }
        
    constexpr auto attachmentReference2() -> VkAttachmentReference2
    {
        return {
            .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
            .pNext = nullptr,
            .attachment = VK_ATTACHMENT_UNUSED,
            .layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .aspectMask = VK_IMAGE_ASPECT_NONE
        };
    }

    constexpr auto subpassDescription2() -> VkSubpassDescription2
    {
        return {
            .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
            .pNext = nullptr,
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .viewMask = 0,
            .inputAttachmentCount = 0,
            .pInputAttachments = nullptr,
            .colorAttachmentCount = 0,
            .pColorAttachments = nullptr,
            .pResolveAttachments = nullptr,
            .pDepthStencilAttachment = nullptr,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = nullptr
        };
    }
} // namespace mxc
