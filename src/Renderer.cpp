#include "Renderer.h"
#include "VulkanCommon.h"

#include <logging.h>
#include <cstdlib>
#include <cstring>
#include <vector>

#define MXC_VULKAN_CONTEXT_INL_IMPLEMENTATION
#include <VulkanContext.inl>
#include <stdio.h>

namespace mxc 
{

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

    auto vulkanResultToString(VkResult res) -> char const*
    {
        if(res == VK_SUCCESS) return "VK_SUCCESS";
        if(res == VK_NOT_READY) return "VK_NOT_READY";
        if(res == VK_TIMEOUT) return "VK_TIMEOUT";
        if(res == VK_EVENT_SET) return "VK_EVENT_SET";
        if(res == VK_EVENT_RESET) return "VK_EVENT_RESET";
        if(res == VK_INCOMPLETE) return "VK_INCOMPLETE";
        if(res == VK_ERROR_OUT_OF_HOST_MEMORY) return "VK_ERROR_OUT_OF_HOST_MEMORY";
        if(res == VK_ERROR_OUT_OF_DEVICE_MEMORY) return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        if(res == VK_ERROR_INITIALIZATION_FAILED) return "VK_ERROR_INITIALIZATION_FAILED";
        if(res == VK_ERROR_DEVICE_LOST) return "VK_ERROR_DEVICE_LOST";
        if(res == VK_ERROR_MEMORY_MAP_FAILED) return "VK_ERROR_MEMORY_MAP_FAILED";
        if(res == VK_ERROR_LAYER_NOT_PRESENT) return "VK_ERROR_LAYER_NOT_PRESENT";
        if(res == VK_ERROR_EXTENSION_NOT_PRESENT) return "VK_ERROR_EXTENSION_NOT_PRESENT";
        if(res == VK_ERROR_FEATURE_NOT_PRESENT) return "VK_ERROR_FEATURE_NOT_PRESENT";
        if(res == VK_ERROR_INCOMPATIBLE_DRIVER) return "VK_ERROR_INCOMPATIBLE_DRIVER";
        if(res == VK_ERROR_TOO_MANY_OBJECTS) return "VK_ERROR_TOO_MANY_OBJECTS";
        if(res == VK_ERROR_FORMAT_NOT_SUPPORTED) return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        if(res == VK_ERROR_FRAGMENTED_POOL) return "VK_ERROR_FRAGMENTED_POOL";
        if(res == VK_ERROR_UNKNOWN) return "VK_ERROR_UNKNOWN";
        if(res == VK_ERROR_SURFACE_LOST_KHR) return "VK_ERROR_SURFACE_LOST_KHR";
        // Provided by VK_KHR_surface
        if(res == VK_ERROR_NATIVE_WINDOW_IN_USE_KHR) return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        // Provided by VK_KHR_swapchain
        if(res == VK_SUBOPTIMAL_KHR) return "VK_SUBOPTIMAL_KHR";
        // Provided by VK_KHR_swapchain
        if(res == VK_ERROR_OUT_OF_DATE_KHR) return "VK_ERROR_OUT_OF_DATE_KHR";
        return "unknown";
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
#ifndef _DEBUG
    #error "fhoiewrhf"
#endif
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
        if (!m_ctx.device.create(&m_ctx, requirements))
        {
            MXC_ERROR("Failed to create device!");
            return false;
        }

        // Swapchain
        m_ctx.swapchain.initDeviceFunctionPointers(&m_ctx);
        m_ctx.framebufferWidth = config.windowWidth;
        m_ctx.framebufferHeight = config.windowHeight;
        m_ctx.swapchain.create(&m_ctx, m_ctx.framebufferWidth, m_ctx.framebufferHeight, /*vsync*/true);

        // Create command buffers.
//        create_command_buffers(context);
//
//        // Create sync objects.
//        context->image_available_semaphores = darray_reserve(VkSemaphore, context->swapchain.max_frames_in_flight);
//        context->queue_complete_semaphores = darray_reserve(VkSemaphore, context->swapchain.max_frames_in_flight);
//
//        for (u8 i = 0; i < context->swapchain.max_frames_in_flight; ++i) {
//            VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
//            vkCreateSemaphore(context->device.logical_device, &semaphore_create_info, context->allocator, &context->image_available_semaphores[i]);
//            vkCreateSemaphore(context->device.logical_device, &semaphore_create_info, context->allocator, &context->queue_complete_semaphores[i]);
//
//            // Create the fence in a signaled state, indicating that the first frame has already been "rendered".
//            // This will prevent the application from waiting indefinitely for the first frame to render since it
//            // cannot be rendered until a frame is "rendered" before it.
//            VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
//            fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
//            VK_CHECK(vkCreateFence(context->device.logical_device, &fence_create_info, context->allocator, &context->in_flight_fences[i]));
//        }
//
//        // In flight fences should not yet exist at this point, so clear the list. These are stored in pointers
//        // because the initial state should be 0, and will be 0 when not in use. Acutal fences are not owned
//        // by this list.
//        for (u32 i = 0; i < context->swapchain.image_count; ++i) {
//            context->images_in_flight[i] = 0;
//        }
//
//        // Create buffers
//
//        // Geometry vertex buffer
//        const u64 vertex_buffer_size = sizeof(vertex_3d) * 1024 * 1024;
//        if (!renderer_renderbuffer_create(RENDERBUFFER_TYPE_VERTEX, vertex_buffer_size, true, &context->object_vertex_buffer)) {
//            KERROR("Error creating vertex buffer.");
//            return false;
//        }
//        renderer_renderbuffer_bind(&context->object_vertex_buffer, 0);
//
//        // Geometry index buffer
//        const u64 index_buffer_size = sizeof(u32) * 1024 * 1024;
//        if (!renderer_renderbuffer_create(RENDERBUFFER_TYPE_INDEX, index_buffer_size, true, &context->object_index_buffer)) {
//            KERROR("Error creating index buffer.");
//            return false;
//        }
//        renderer_renderbuffer_bind(&context->object_index_buffer, 0);
//
//        // Mark all geometries as invalid
//        for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
//            context->geometries[i].id = INVALID_ID;
//        }
//
//        KINFO("Vulkan renderer initialized successfully.");
        return true;
    }

    auto Renderer::cleanup() -> void
    {
        vkDeviceWaitIdle(m_ctx.device.logical);
//        // Cold-cast the context
//        vulkan_context* context = (vulkan_context*)plugin->internal_context;
//        vkDeviceWaitIdle(context->device.logical_device);
//
//        // Destroy in the opposite order of creation.
//        // Destroy buffers
//        renderer_renderbuffer_destroy(&context->object_vertex_buffer);
//        renderer_renderbuffer_destroy(&context->object_index_buffer);
//
//        // Sync objects
//        for (u8 i = 0; i < context->swapchain.max_frames_in_flight; ++i) {
//            if (context->image_available_semaphores[i]) {
//                vkDestroySemaphore(
//                    context->device.logical_device,
//                    context->image_available_semaphores[i],
//                    context->allocator);
//                context->image_available_semaphores[i] = 0;
//            }
//            if (context->queue_complete_semaphores[i]) {
//                vkDestroySemaphore(
//                    context->device.logical_device,
//                    context->queue_complete_semaphores[i],
//                    context->allocator);
//                context->queue_complete_semaphores[i] = 0;
//            }
//            vkDestroyFence(context->device.logical_device, context->in_flight_fences[i], context->allocator);
//        }
//        darray_destroy(context->image_available_semaphores);
//        context->image_available_semaphores = 0;
//
//        darray_destroy(context->queue_complete_semaphores);
//        context->queue_complete_semaphores = 0;
//
//        // Command buffers
//        for (u32 i = 0; i < context->swapchain.image_count; ++i) {
//            if (context->graphics_command_buffers[i].handle) {
//                vulkan_command_buffer_free(
//                    context,
//                    context->device.graphics_command_pool,
//                    &context->graphics_command_buffers[i]);
//                context->graphics_command_buffers[i].handle = 0;
//            }
//        }
//        darray_destroy(context->graphics_command_buffers);
//        context->graphics_command_buffers = 0;

        // Swapchain
        MXC_DEBUG("Destroying the Swapchain");
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
}
