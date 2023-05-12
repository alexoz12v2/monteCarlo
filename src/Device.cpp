// most of the code is inspired from https://github.com/travisvroman/kohi/
#include "Device.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <array>

#include "VulkanContext.inl"
#include "logging.h"

namespace mxc
{
    auto Device::selectPhysicalDevice(VulkanContext* ctx, PhysicalDeviceRequirements const& requirements) -> bool
    {
        uint32_t physicalDevice_count = 0;
        VK_CHECK(vkEnumeratePhysicalDevices(ctx->instance, &physicalDevice_count, 0));
        if (physicalDevice_count == 0) 
        {
            MXC_ERROR("No devices which support Vulkan were found.");
            return false;
        }

        // Iterate physical devices to find one that fits the bill.
        VkPhysicalDevice physicalDevices[32];
        VK_CHECK(vkEnumeratePhysicalDevices(ctx->instance, &physicalDevice_count, physicalDevices));
        for (uint32_t i = 0; i < physicalDevice_count; ++i) 
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physicalDevices[i], &properties);

            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(physicalDevices[i], &features);

            VkPhysicalDeviceMemoryProperties memory;
            vkGetPhysicalDeviceMemoryProperties(physicalDevices[i], &memory);

            MXC_INFO("Evaluating device: '%s', index %u.", properties.deviceName, i);

            PhysicalDeviceQueueFamiliesSupport queueFamiliesInfo = {};
            bool result = checkPhysicalDeviceRequirements(ctx, physicalDevices[i], ctx->surface, requirements, properties, features, &queueFamiliesInfo, &swapchainSupport);

            if (result) {
                MXC_INFO("Selected device: '%s'.", properties.deviceName);
                // GPU type, etc.
                switch (properties.deviceType) {
                    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                        MXC_INFO("GPU type is Unknown.");
                        break;
                    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                        MXC_INFO("GPU type is Integrated.");
                        break;
                    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                        MXC_INFO("GPU type is Descrete.");
                        break;
                    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                        MXC_INFO("GPU type is Virtual.");
                        break;
                    case VK_PHYSICAL_DEVICE_TYPE_CPU:
                        MXC_INFO("GPU type is CPU.");
                        break;
                }

                MXC_INFO(
                    "GPU Driver version: %d.%d.%d",
                    VK_VERSION_MAJOR(properties.driverVersion),
                    VK_VERSION_MINOR(properties.driverVersion),
                    VK_VERSION_PATCH(properties.driverVersion));

                // Vulkan API version.
                MXC_INFO(
                    "Vulkan API version: %d.%d.%d",
                    VK_VERSION_MAJOR(properties.apiVersion),
                    VK_VERSION_MINOR(properties.apiVersion),
                    VK_VERSION_PATCH(properties.apiVersion));

                // Memory information
                for (uint32_t j = 0; j < memory.memoryHeapCount; ++j) {
                    float memorySizeGiB = (((float)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
                    if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                        MXC_INFO("Local GPU memory: %.2f GiB", memorySizeGiB);
                    } else {
                        MXC_INFO("Shared System memory: %.2f GiB", memorySizeGiB);
                    }
                }

                ctx->device.physical = physicalDevices[i];
                ctx->device.queueFamilies.graphics= queueFamiliesInfo.graphicsFamily;
                ctx->device.queueFamilies.present = queueFamiliesInfo.presentFamily;
                ctx->device.queueFamilies.transfer= queueFamiliesInfo.transferFamily;
                ctx->device.queueFamilies.compute = queueFamiliesInfo.computeFamily;

                // Keep a copy of properties, features and memory info for later use. maybe
                ctx->device.properties = properties;
                ctx->device.features = features;
                ctx->device.memory = memory;
                break;
            }
        }

        // Ensure a device was selected
        if (physical == VK_NULL_HANDLE) {
            MXC_ERROR("No physical devices were found which meet the requirements.");
            return false;
        }

        MXC_INFO("Physical device selected.");
        return true;
    }

    auto Device::destroy(VulkanContext* ctx) -> bool
    {
        // Unset queues
        graphicsQueue = VK_NULL_HANDLE;
        presentQueue = VK_NULL_HANDLE;
        transferQueue = VK_NULL_HANDLE;

        MXC_INFO("Destroying command pools...");
        vkDestroyCommandPool(logical, commandPool, nullptr);
        commandPool = VK_NULL_HANDLE;

        MXC_INFO("Destroying VMA allocator...");
        vmaDestroyAllocator(vmaAllocator);

        MXC_INFO("Destroying logical device...");
        vkDestroyDevice(logical, nullptr);
        logical = VK_NULL_HANDLE;

        physical = VK_NULL_HANDLE;

        queueFamilies.graphics= -1;
        queueFamilies.present = -1;
        queueFamilies.transfer= -1;
    }

    auto Device::create(VulkanContext* ctx, PhysicalDeviceRequirements const& requirements) -> bool
    {
        if (!selectPhysicalDevice(ctx, requirements)) 
            return false;

        MXC_INFO("Creating logical device...");
        // NOTE: Do not create additional queues for shared queue family indices.
        std::array<uint32_t, QUEUE_FAMILIES_COUNT> uniqueFamilies{UINT32_MAX};
        uint32_t uniqueFamily_count = 0;
        for (uint32_t i = 0; i != QUEUE_FAMILIES_COUNT; ++i)
        {
            // if the current family is equal to any of the previously seen ones, then it is not unique
            bool seen = false;
            for (uint32_t j = 0; j != uniqueFamily_count; ++j)
            {
                if (queueFamilyIndices[i] == uniqueFamilies[j])
                {
                    seen = true;
                    break;
                }
            }
            
            if (!seen)
            {
                uniqueFamilies[uniqueFamily_count++] = queueFamilyIndices[i];
            }
        }
        
        // bool presentSharesGraphicsQueue = context->device.graphics_queue_index == context->device.present_queue_index;
        // bool transferSharesGraphicsQueue = context->device.graphics_queue_index == context->device.transfer_queue_index;
        // u32 index_count = 1;
        // if (!present_shares_graphics_queue) {
        //     index_count++;
        // }
        // if (!transfer_shares_graphics_queue) {
        //     index_count++;
        // }
        // u32 indices[32];
        // u8 index = 0;
        // indices[index++] = context->device.graphics_queue_index;
        // if (!present_shares_graphics_queue) {
        //     indices[index++] = context->device.present_queue_index;
        // }
        // if (!transfer_shares_graphics_queue) {
        //     indices[index++] = context->device.transfer_queue_index;
        // }

        VkDeviceQueueCreateInfo queueCreateInfos[QUEUE_FAMILIES_COUNT];
        float queue_priority = 1.0f;
        for (uint32_t i = 0; i < uniqueFamily_count; ++i) {
            queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfos[i].queueFamilyIndex = uniqueFamilies[i];
            queueCreateInfos[i].queueCount = 1;
            queueCreateInfos[i].flags = 0;
            queueCreateInfos[i].pNext = nullptr;
            queueCreateInfos[i].pQueuePriorities = &queue_priority;
        }

        // Request device features. TODO maybe
        // VkPhysicalDeviceFeatures device_features = {};
        // device_features.samplerAnisotropy = VK_TRUE;  // Request anistrophy

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = uniqueFamily_count;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
        deviceCreateInfo.pEnabledFeatures = nullptr;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requirements.extensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = requirements.extensions.data();

        // Deprecated and ignored, so pass nothing.
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.ppEnabledLayerNames = nullptr;

        // Create the device.
        VK_CHECK(vkCreateDevice(physical, &deviceCreateInfo, nullptr, &logical));

        MXC_INFO("Logical device created.");

        // Get queues.
        vkGetDeviceQueue(logical, queueFamilies.graphics, /*queue index*/0, &graphicsQueue);
        vkGetDeviceQueue(logical, queueFamilies.present, 0, &presentQueue);
        vkGetDeviceQueue(logical, queueFamilies.transfer, 0, &transferQueue);
        vkGetDeviceQueue(logical, queueFamilies.compute, 0, &computeQueue);
        MXC_INFO("Queues obtained.");

        // Create command pool for graphics queue. TODO create a queue for each unique family
        VkCommandPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolCreateInfo.queueFamilyIndex = queueFamilies.graphics;
        poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VK_CHECK(vkCreateCommandPool(logical, &poolCreateInfo, nullptr, &commandPool));
        MXC_INFO("command pool created.");
        
        // Create VMA Allocator
        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        allocatorCreateInfo.physicalDevice = physical;
        allocatorCreateInfo.device = logical;
        allocatorCreateInfo.instance = ctx->instance;

        VK_CHECK(vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator));
        MXC_INFO("VMA allocator created.");

        return true;
    }
    
    auto Device::querySwapchainSupport(VulkanContext* ctx, VkSurfaceKHR surface) -> bool
    {
        internalQuerySwapchainSupport(ctx, physical, surface, &swapchainSupport);
    }

    auto Device::checkPhysicalDeviceRequirements(
        VulkanContext* ctx, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, PhysicalDeviceRequirements const& requirements,
        VkPhysicalDeviceProperties const& properties, VkPhysicalDeviceFeatures const& features,
        PhysicalDeviceQueueFamiliesSupport* outFamilySupport, SwapchainSupport* outSwapchainSupport) const -> bool
    {
        // Discrete GPU?
        if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) 
        {
                MXC_INFO("Device is not a discrete GPU, and one is required. Skipping.");
                return false;
        }

        uint32_t queueFamilyProp_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyProp_count, 0);
        VkQueueFamilyProperties queueFamiliesProps[32];
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyProp_count, queueFamiliesProps);

        // Look at each queue and see what queues it supports
        MXC_INFO("Graphics | Present | Compute | Transfer | Name");
        uint8_t minTransferScore = 255;
        for (uint32_t i = 0; i < queueFamilyProp_count; ++i) 
        {
            // taking a transfer queue which is not graphics queue is more optimal
            uint8_t currentTransferScore = 0;

            // Graphics queue?
            if (outFamilySupport->graphicsFamily == -1 && queueFamiliesProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) 
            {
                outFamilySupport->graphicsFamily = i;
                ++currentTransferScore;

                // If also a present queue, this prioritizes grouping of the 2.
                VkBool32 supportsPresent = VK_FALSE;
                VK_CHECK(ctx->swapchain.fpGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent));
                if (supportsPresent) 
                {
                    outFamilySupport->presentFamily = i;
                    ++currentTransferScore;
                }
            }

            // Compute queue?
            if (queueFamiliesProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) 
            {
                outFamilySupport->computeFamily = i;
                ++currentTransferScore;
            }

            // Transfer queue?
            if (queueFamiliesProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT) 
            {
                // Take the index if it is the current lowest. This increases the
                // liklihood that it is a dedicated transfer queue.
                if (currentTransferScore <= minTransferScore) 
                {
                    minTransferScore = currentTransferScore;
                    outFamilySupport->transferFamily = i;
                }
            }
        }

        // If a present queue hasn't been found, iterate again and take the first one.
        // This should only happen if there is a queue that supports graphics but NOT
        // present.
        if (outFamilySupport->presentFamily == -1) 
        {
            for (uint32_t i = 0; i < queueFamilyProp_count; ++i) 
            {
                VkBool32 supportsPresent = VK_FALSE;
                VK_CHECK(ctx->swapchain.fpGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent));
                if (supportsPresent) 
                {
                    outFamilySupport->presentFamily = i;

                    // If they differ, bleat about it and move on. This is just here for troubleshooting
                    // purposes.
                    if (outFamilySupport->presentFamily != outFamilySupport->graphicsFamily) 
                    {
                        MXC_WARN("Warning: Different queue index used for present vs graphics: %u.", i);
                    }
                    break;
                }
            }
        }

        // Print out some info about the device
        MXC_INFO("       %d |       %d |       %d |        %d | %s",
              outFamilySupport->graphicsFamily != -1,
              outFamilySupport->presentFamily != -1,
              outFamilySupport->transferFamily != -1,
              outFamilySupport->computeFamily != -1,
              properties.deviceName);

        if (!(outFamilySupport->graphicsFamily != -1 &&
              outFamilySupport->presentFamily != -1 &&
              outFamilySupport->transferFamily != -1 &&
              outFamilySupport->computeFamily != -1)) 
        {
            MXC_INFO("Device doesn't meet queue requirements, moving on...");
            return false;
        }

        MXC_INFO("Device meets queue requirements.");
        MXC_TRACE("Graphics Family Index: %i", outFamilySupport->graphicsFamily);
        MXC_TRACE("Present Family Index:  %i", outFamilySupport->presentFamily);
        MXC_TRACE("Transfer Family Index: %i", outFamilySupport->transferFamily);
        MXC_TRACE("Compute Family Index:  %i", outFamilySupport->computeFamily);

        // Query swapchain support.
        internalQuerySwapchainSupport(ctx, physicalDevice, surface, outSwapchainSupport);

        if (outSwapchainSupport->formats.size() < 1 || outSwapchainSupport->presentModes.size() < 1) 
        {
            MXC_INFO("Required swapchain support not present, skipping device.");
            return false;
        }

        // Device extensions.
        if (requirements.extensions.size() != 0) 
        {
            uint32_t availableExtension_count = 0;
            std::vector<VkExtensionProperties> availableExtensions;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableExtension_count, nullptr));
            if (availableExtension_count == 0)
            {
                MXC_INFO("physical device doesn't meet device extension requirements, moving on...");
            }

            availableExtensions.resize(availableExtension_count);
            VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableExtension_count, availableExtensions.data()));

            for (uint32_t i = 0; i < requirements.extensions.size(); ++i) 
            {
                bool found = false;
                for (uint32_t j = 0; j < availableExtension_count; ++j) 
                {
                    if (0 == strcmp(requirements.extensions[i], availableExtensions[j].extensionName)) 
                    {
                        found = true;
                        break;
                    }
                }

                if (!found) 
                {
                    MXC_INFO("Required extension not found: '%s', skipping device.", requirements.extensions[i]);
                    return false;
                }
            }
        }

        // Features TODO

        // Device meets all requirements.
        return true;
    }
    
    auto Device::internalQuerySwapchainSupport(VulkanContext* ctx, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, 
                                               SwapchainSupport* outSwapchainSupport) const -> bool
    {
        // Surface capabilities
        VK_CHECK(ctx->swapchain.fpGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &outSwapchainSupport->surfCaps));

        // Surface formats
        uint32_t formatCount;
        VK_CHECK(ctx->swapchain.fpGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr));

        if (formatCount != 0) 
        {
            outSwapchainSupport->formats.resize(formatCount);
            VK_CHECK(ctx->swapchain.fpGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, outSwapchainSupport->formats.data()));
        }

        // Present modes
        uint32_t presentModeCount;
        VK_CHECK(ctx->swapchain.fpGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));
        if (presentModeCount != 0) 
        {
            outSwapchainSupport->presentModes.resize(presentModeCount);
            VK_CHECK(ctx->swapchain.fpGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, outSwapchainSupport->presentModes.data()));
        }

        return true;
    }
}
