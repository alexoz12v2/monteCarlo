#ifndef MXC_INITIALIZERS_HPP
#define MXC_INITIALIZERS_HPP

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <utility>
#include <limits>

namespace mxc
{
bool constexpr debugMode 
{
#ifdef NDEBUG
    false
#else
    true
#endif
};

}

namespace mxc::vkdefs
{
    consteval auto applicationInfo() -> VkApplicationInfo
    {
        return VkApplicationInfo{
    		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    		.pNext = nullptr,
    		.pApplicationName = "vulkan renderer",
    		.applicationVersion = 1u,
    		.pEngineName = "vulkan renderer engine",
    		.engineVersion = 1u,
    		.apiVersion = VK_API_VERSION_1_3
        };
    }

    consteval auto instanceCreateInfo() -> VkInstanceCreateInfo
    {
        return {
    		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    		.pNext = nullptr, // as of vulkan 1.3, there are no pNext for instance
    		.flags = 0u, // as of vulkan 1.3, there are no flags
    		.pApplicationInfo = nullptr,
    		.enabledLayerCount = 0u,
    		.ppEnabledLayerNames = nullptr,
    		.enabledExtensionCount = 0u,
    		.ppEnabledExtensionNames = nullptr
        };
    }

#ifndef NDEBUG
	// workaround to have static constexpr stuff
	struct debugUtilsMessengerCreateInfo_dbg_StaticData
	{
		static inline VkDebugUtilsMessageSeverityFlagsEXT constexpr warnAndError = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT 
																	  | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		static inline VkDebugUtilsMessageTypeFlagsEXT constexpr generalAndValidation = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
																		  | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	};
#endif

    constexpr auto debugUtilsMessengerCreateInfo_dbg() -> std::conditional_t<debugMode, VkDebugUtilsMessengerCreateInfoEXT, void>
    {
#ifndef NDEBUG
        return VkDebugUtilsMessengerCreateInfoEXT{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext = nullptr,
            .flags = 0,
            .messageSeverity = debugUtilsMessengerCreateInfo_dbg_StaticData::warnAndError,
            .messageType = debugUtilsMessengerCreateInfo_dbg_StaticData::generalAndValidation,
            .pfnUserCallback = nullptr,
            .pUserData = nullptr
        };
#else
        return;
#endif
    }


    auto VKAPI_PTR defaultDebugUtilsMessengerCallback([[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                      [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                      [[maybe_unused]] VkDebugUtilsMessengerCallbackDataEXT const * pCallbackData,
                                                      [[maybe_unused]] void* pUserData)
        -> VkBool32
    {
        if constexpr (debugMode)
        {
            // TODO log
        }

        return VK_FALSE;
    }
    
    consteval auto deviceCreateInfo() -> VkDeviceCreateInfo 
    {
        return {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueCreateInfoCount = 0, // to be set from user
            .pQueueCreateInfos = nullptr, // to be set from user
            .enabledLayerCount = 0, // deprecated
            .ppEnabledLayerNames = nullptr, // deprecated
            .enabledExtensionCount = 0, // to be set from user
            .ppEnabledExtensionNames = nullptr, // to be set from user
            .pEnabledFeatures  = nullptr // VulkanRenderer instances always use VKPhysicalDeviceFeatures2, which are specified in pNext
        };
    }

    consteval auto deviceQueueCreateInfo() -> VkDeviceQueueCreateInfo
    {
        return {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr, // set by user, maybe
            .flags = 0, // VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT only if feature protected is enabled. TODO support that if you need it 
            .queueFamilyIndex = std::numeric_limits<uint32_t>::max(), // to be set from user
            .queueCount = 0, // to be set from user
            .pQueuePriorities = nullptr, // to be set from user
        };
    }

    consteval auto deviceQueueInfo2() -> VkDeviceQueueInfo2
    {
        return {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
            .pNext = nullptr,
            .flags = 0, // VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT only if feature protected is enabled. TODO support that if you need it 
            .queueFamilyIndex = std::numeric_limits<uint32_t>::max(), // to be set from user
            .queueIndex = std::numeric_limits<uint32_t>::max() // to be set from user
        };
    }

    consteval auto vmaAllocatorCreateInfo() -> VmaAllocatorCreateInfo
    {
        return {
            .flags = 0,
            .physicalDevice = VK_NULL_HANDLE,
            .device = VK_NULL_HANDLE,
            .preferredLargeHeapBlockSize = 0, // optional
            .pAllocationCallbacks = nullptr, // VkAllocationCallbacks
            .pDeviceMemoryCallbacks = nullptr, // optional
            .pHeapSizeLimit = nullptr,
            .pVulkanFunctions = nullptr,
            .instance = VK_NULL_HANDLE,
            .vulkanApiVersion = VK_API_VERSION_1_3,
            .pTypeExternalMemoryHandleTypes = nullptr
        };
    }

    consteval auto renderPassBeginInfo(VkRenderPass renderPass = VK_NULL_HANDLE, VkFramebuffer framebuffer = VK_NULL_HANDLE) -> VkRenderPassBeginInfo
    {
        return {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = nullptr,
            .renderPass = renderPass,
            .framebuffer = framebuffer,
            .renderArea = VkRect2D(),
            .clearValueCount = 0,
            .pClearValues = nullptr
        };
    }

}

#endif // MXC_INITIALIZERS_HPP
