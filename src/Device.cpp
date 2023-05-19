// most of the code is inspired from https://github.com/travisvroman/kohi/
#include "Device.h"

#include "VulkanContext.inl"
#include "Buffer.h"
#include "Image.h"
#include "CommandBuffer.h"
#include "logging.h"

// TODO create macros for pragmas
#define VMA_IMPLEMENTATION
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#include <vk_mem_alloc.h>
#pragma clang diagnostic pop

#include <array>
#include <utility>

namespace mxc
{
    constexpr auto chooseVmaAllocationCI(BufferType_t type, bool preferCPUmemory, bool GPUonlyResource) -> VmaAllocationCreateInfo;
    constexpr auto chooseMemoryPropertyFlags(StorageUniformMemoryOptions options) -> VkMemoryPropertyFlags;

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
            SwapchainSupport swapSup;
            bool result = checkPhysicalDeviceRequirements(ctx, physicalDevices[i], ctx->surface, requirements, properties, features, &queueFamiliesInfo, &swapSup);
            swapchainSupport = std::move(swapSup);

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

    auto Device::destroy([[maybe_unused]] VulkanContext* ctx) -> bool
    {
        // free all allocations.
        if (m_allocations.size() != 0)
        {
            MXC_WARN("Freeing %zu allocations on device. This may mean that some Vulkan Handles have not been destroyed yet...", m_allocations.size());
        }
        for (uint32_t i = 0; i != m_allocations.size(); ++i)
        {
            if (!m_allocations[i].freed)
                vmaFreeMemory(vmaAllocator, m_allocations[i].allocation);
        }

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

        return true;
    }

    auto Device::create(VulkanContext* ctx, PhysicalDeviceRequirements const& requirements) -> bool
    {
        m_allocations.reserve(INITIAL_ALLOCATIONS_VECTOR_CAPACITY);

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

        VkDeviceQueueCreateInfo queueCreateInfos[QUEUE_FAMILIES_COUNT];
        float queue_priority = 1.0f;
        for (uint32_t i = 0; i < uniqueFamily_count; ++i) {
            queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfos[i].queueFamilyIndex = uniqueFamilies[i];
            queueCreateInfos[i].queueCount = 1; // TODO maybe check for feature for more?
            queueCreateInfos[i].flags = 0;
            queueCreateInfos[i].pNext = nullptr;
            queueCreateInfos[i].pQueuePriorities = &queue_priority;
        }

        // Request device features. TODO maybe
        // VkPhysicalDeviceFeatures device_features = {};
        // device_features.samplerAnisotropy = VK_TRUE;  // Request anistropy

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
    
    auto Device::checkPhysicalDeviceRequirements(
        VulkanContext* ctx, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, PhysicalDeviceRequirements const& requirements,
        VkPhysicalDeviceProperties const& properties, [[maybe_unused]] VkPhysicalDeviceFeatures const& features,
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
        assert(queueFamilyProp_count <= 32);
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
                assert(ctx->swapchain.fpGetPhysicalDeviceSurfaceSupportKHR);
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
        querySwapchainSupport(ctx, physicalDevice, surface, outSwapchainSupport);

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
    
    auto Device::querySwapchainSupport(VulkanContext* ctx, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, 
                                               SwapchainSupport* outSwapchainSupport) const -> bool
    {
        // Surface capabilities
        assert(ctx->swapchain.fpGetPhysicalDeviceSurfaceCapabilitiesKHR && ctx->swapchain.fpGetPhysicalDeviceSurfaceFormatsKHR
               && ctx->swapchain.fpGetPhysicalDeviceSurfacePresentModesKHR);
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

    auto Device::updateSwapchainSupport(VulkanContext* ctx) -> bool
    {
        SwapchainSupport swapSup;
        querySwapchainSupport(ctx, physical, ctx->surface, &swapSup);
        swapchainSupport.surfCaps     = swapSup.surfCaps;
        swapchainSupport.formats      = std::move(swapSup.formats);
        swapchainSupport.presentModes = std::move(swapSup.presentModes);
        return true;
    }

    auto Device::selectDepthFormat(VkFormat* outDepthFormat, DepthFormatProperties* outDepthFormatProps) const -> bool
    {
        // Format candidates
        // Since all depth formats may be optional, we need to find a suitable depth format to use
        // Start with the highest precision packed format
        std::array candidateDepthFormats = {(VkFormat)
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D24_UNORM_S8_UINT,
                VK_FORMAT_D16_UNORM_S8_UINT,
                VK_FORMAT_D16_UNORM
        };

        // requested characteristics. TODO configurable?
        VkFormatFeatureFlagBits flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;

        for (uint32_t i = 0; i < candidateDepthFormats.size(); ++i) 
        {
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(physical, candidateDepthFormats[i], &properties);

            if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & flags) == flags) 
            {
                *outDepthFormat = candidateDepthFormats[i];
                if (outDepthFormatProps) *outDepthFormatProps =
                    (candidateDepthFormats[i] == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                    candidateDepthFormats[i] == VK_FORMAT_D32_SFLOAT ||
                    candidateDepthFormats[i] == VK_FORMAT_D24_UNORM_S8_UINT ||
                    candidateDepthFormats[i] == VK_FORMAT_D16_UNORM_S8_UINT) ? DepthFormatProperties::SUPPORTS_STENCIL : DepthFormatProperties::NONE;
                return true;
            } 
            else if (tiling == VK_IMAGE_TILING_LINEAR && (properties.optimalTilingFeatures & flags) == flags) 
            {
                *outDepthFormat = candidateDepthFormats[i];
                if (outDepthFormatProps) *outDepthFormatProps =
                    (candidateDepthFormats[i] == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                    candidateDepthFormats[i] == VK_FORMAT_D32_SFLOAT ||
                    candidateDepthFormats[i] == VK_FORMAT_D24_UNORM_S8_UINT ||
                    candidateDepthFormats[i] == VK_FORMAT_D16_UNORM_S8_UINT) ? DepthFormatProperties::SUPPORTS_STENCIL : DepthFormatProperties::NONE;
                return true;
            }
        }

        return false;
    }

    auto Device::createBuffer(Buffer* inOutBuffer, StorageUniformMemoryOptions options) -> bool
    {
        MXC_ASSERT(inOutBuffer, "createBuffer function requires a valid blank Buffer object");
        MXC_ASSERT(inOutBuffer->type != BufferType_v::INVALID, "Cannot free an invalid buffer");
        MXC_ASSERT(logical != VK_NULL_HANDLE, "Cannot create a buffer on a device not created yet!");

        MXC_DEBUG("Creating a Buffer...");
        VkBufferCreateInfo bufferCreateInfo {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferCreateInfo.size = inOutBuffer->size;

        if ((inOutBuffer->type & BufferType_v::VERTEX) == BufferType_v::VERTEX)  
        {
            bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            inOutBuffer->memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }
        else if ((inOutBuffer->type & BufferType_v::INDEX) == BufferType_v::INDEX)
        {
            bufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            inOutBuffer->memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }
        else if ((inOutBuffer->type & BufferType_v::UNIFORM) == BufferType_v::UNIFORM)
        {
            bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            inOutBuffer->memoryPropertyFlags = chooseMemoryPropertyFlags(options);
        }
        else if ((inOutBuffer->type & BufferType_v::UNIFORM_TEXEL) == BufferType_v::UNIFORM_TEXEL)
        {
            bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            inOutBuffer->memoryPropertyFlags = chooseMemoryPropertyFlags(options);
        }
        else if ((inOutBuffer->type & BufferType_v::STAGING) == BufferType_v::STAGING)
        {
            bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            inOutBuffer->memoryPropertyFlags = chooseMemoryPropertyFlags(options);
        }

        // buffers can be read-write too, therefore no else if here
        if ((inOutBuffer->type & BufferType_v::STORAGE) == BufferType_v::STORAGE)
        {
            bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            inOutBuffer->memoryPropertyFlags = chooseMemoryPropertyFlags(options);
        }
        else if ((inOutBuffer->type & BufferType_v::STORAGE_TEXEL) == BufferType_v::STORAGE_TEXEL)
        {
            bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            inOutBuffer->memoryPropertyFlags = chooseMemoryPropertyFlags(options);
        }
        
        VK_CHECK(vkCreateBuffer(logical, &bufferCreateInfo, nullptr, &inOutBuffer->handle));

        // memory index, memory requirements, mapped
        // Note TODO: might need to make this configurable
        MXC_DEBUG("Allocating memory for the Buffer...");
        VmaAllocationCreateInfo allocationCreateInfo = chooseVmaAllocationCI(inOutBuffer->type, /*preferCPUmemory*/false, /*GPUonlyResource*/true);
        VK_CHECK(vmaFindMemoryTypeIndexForBufferInfo(vmaAllocator, &bufferCreateInfo, &allocationCreateInfo, &inOutBuffer->memoryIndex));

        // TODO use vmaSetAllocationName for debug
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
        VK_CHECK(vmaAllocateMemoryForBuffer(vmaAllocator, inOutBuffer->handle, &allocationCreateInfo, &allocation, &allocationInfo));
        VK_CHECK(vmaBindBufferMemory(vmaAllocator, allocation, inOutBuffer->handle));
        if ((inOutBuffer->type & BufferType_v::STAGING) == BufferType_v::STAGING) inOutBuffer->mapped = allocationInfo.pMappedData;

        inOutBuffer->allocationIndex = static_cast<uint32_t>(m_allocations.size());
        // m_allocations.emplace_back(allocation, false); error: no matching function for call to 'construct_at'
        m_allocations.push_back({allocation, false});

        MXC_DEBUG("Buffer creation complete.");
        return true;
    }
    
    auto Device::destroyBuffer(Buffer* inOutBuffer) -> void
    {
        // free associated memory TODO add more debug info  
        MXC_ASSERT(inOutBuffer, "destroyBuffer function requires a valid Buffer object");
        MXC_ASSERT(inOutBuffer->type != BufferType_v::INVALID, "Cannot free an invalid buffer");
        MXC_DEBUG("Destroying a Buffer");
        vmaFreeMemory(vmaAllocator, m_allocations[inOutBuffer->allocationIndex].allocation);
        m_allocations[inOutBuffer->allocationIndex].freed = true;
        inOutBuffer->allocationIndex = UINT32_MAX;
        inOutBuffer->mapped = nullptr;
        inOutBuffer->type = BufferType_v::INVALID; // these 2 need to be reset manually if you want to reuse this buffer again
        inOutBuffer->size = 0;

        vkDestroyBuffer(logical, inOutBuffer->handle, nullptr);
        inOutBuffer->handle = VK_NULL_HANDLE;
    }

    auto Device::flushCommandBuffer(CommandBuffer* pCmdBuf, CommandType type) -> bool
    {
        MXC_ASSERT(pCmdBuf && pCmdBuf->isExecutable(), "Invalid command buffer given for submission");

        VkFence fence;
        VkFenceCreateInfo fenceInfo {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VK_CHECK(vkCreateFence(logical, &fenceInfo, nullptr, &fence));
        
        VkQueue queue;
        switch (type)
        {
            case CommandType::GRAPHICS: queue = graphicsQueue;
            case CommandType::TRANSFER: queue = transferQueue;
            case CommandType::COMPUTE:  queue = computeQueue;
        }

        MXC_ASSERT(pCmdBuf->isPrimary(), "Cannot submit a secondary command buffer");
        VkCommandBufferSubmitInfo cmdBufSubmitInfo {};
        cmdBufSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        cmdBufSubmitInfo.commandBuffer = pCmdBuf->handle;
        
        // TODO VK_KHR_performance_query
        VkSubmitInfo2 submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &cmdBufSubmitInfo;

        pCmdBuf->signalSubmit();
        VK_CHECK(vkQueueSubmit2(queue, 1, &submitInfo, fence));

        VK_CHECK(vkWaitForFences(logical, 1, &fence, VK_TRUE, 100000000000/*ns*/)); // configurable? 
        vkDestroyFence(logical, fence, nullptr);
        pCmdBuf->signalCompletion();

        return true;
    }

    auto Device::copyBuffer(VulkanContext* ctx, Buffer const* src, Buffer* dst) -> bool
    {
        MXC_ASSERT(ctx && src && dst, "copyBuffer function requires valid Buffer objects");
        MXC_ASSERT(src->type != BufferType_v::INVALID && dst->type != BufferType_v::INVALID, "Cannot perform copy to/from an invalid buffer");
        MXC_ASSERT(dst->size <= src->size, "Cannot copy a buffer into a smaller one");

        // Note TODO: creating a command buffer each copy op might hurt performance if copy ops are invoked during render loop
        CommandBuffer copyCmdBuf;
        MXC_ASSERT(copyCmdBuf.allocate(ctx), "couldn't allocate copy Command Buffer");
        VkBufferCopy2 bufferCopy {
            .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
            .pNext = nullptr,
            .srcOffset = 0,
            .dstOffset = 0,
            .size = src->size
        };
        VkCopyBufferInfo2 copyBufferInfo {
            .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
            .pNext = nullptr,
            .srcBuffer = src->handle,
            .dstBuffer = dst->handle,
            .regionCount = 1,
            .pRegions = &bufferCopy,
        };

        copyCmdBuf.begin();
        MXC_ASSERT(copyCmdBuf.canRecord(), "Copy Command Buffer is not in recording state");
        vkCmdCopyBuffer2(copyCmdBuf.handle, &copyBufferInfo);
        MXC_ASSERT(copyCmdBuf.end(), "Couldn't end recording of copy Command Buffer");

        flushCommandBuffer(&copyCmdBuf, CommandType::TRANSFER);
        copyCmdBuf.free(ctx);
        
        return true;
    }

    auto Device::createImage(
        VulkanContext* ctx, 
        VkImageTiling tiling, 
        Image* inOutImage, 
        VkImageLayout const* targetLayout, 
        ImageView* inOutView) -> bool
    {
        MXC_ASSERT(inOutImage, "Need a valid blank image to create data for it");

        MXC_DEBUG("Creating an Image");
        VkImageCreateInfo imageCreateInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = inOutImage->type,
            .format = inOutImage->format,
            .extent = inOutImage->extent,
            .mipLevels = inOutImage->mipLevels,
            .arrayLayers = inOutImage->arrayLayers,
            .samples = VK_SAMPLE_COUNT_1_BIT, // TODO multisampling?
            .tiling = tiling,
            .usage = inOutImage->usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        VK_CHECK(vkCreateImage(logical, &imageCreateInfo, nullptr, &inOutImage->handle));

        MXC_DEBUG("Allocating memory for the image");
        VmaAllocationCreateInfo allocationCreateInfo{};
        VK_CHECK(vmaFindMemoryTypeIndexForImageInfo(vmaAllocator, &imageCreateInfo, &allocationCreateInfo, &inOutImage->memoryIndex));

        // TODO use vmaSetAllocationName for debug
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo{};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        VK_CHECK(vmaAllocateMemoryForImage(vmaAllocator, inOutImage->handle, &allocationCreateInfo, &allocation, &allocationInfo));
        VK_CHECK(vmaBindImageMemory(vmaAllocator, allocation, inOutImage->handle));

        inOutImage->allocationIndex = static_cast<uint32_t>(m_allocations.size());
        // m_allocations.emplace_back(allocation, false); error: no matching function for call to 'construct_at'
        m_allocations.push_back({allocation, false});

        MXC_DEBUG("Image Creation complete");

        if (targetLayout)
        {
            MXC_TRACE("Target Layout given to image Creation, performing memory barrier operation");

            CommandBuffer cmdBuf;
            MXC_ASSERT(cmdBuf.allocate(ctx), "Couldn't allocate image memory barrier command buffer");

            cmdBuf.begin();
            MXC_ASSERT(cmdBuf.canRecord(), "Image memory barrier Command Buffer is not in recording state");

            insertImageMemoryBarrier(&cmdBuf, inOutImage, VK_IMAGE_LAYOUT_UNDEFINED, *targetLayout, inOutView->aspectMask);

            MXC_ASSERT(cmdBuf.end(), "Couldn't end recording of image memory barrier Command Buffer");

            flushCommandBuffer(&cmdBuf, CommandType::GRAPHICS);
            cmdBuf.free(ctx);
        }

        // TODO: VkSamplerYcbcrConversionInfo
        if (inOutView)
        {
            createImageView(inOutImage, inOutView);
        }
        
        return true;
    }

    auto Device::destroyImage(Image* inOutImage) -> void
    {
        // free associated memory TODO add more debug info  
        MXC_ASSERT(inOutImage, "destroyImage function requires a valid Image object");
        MXC_DEBUG("Destroying a Image");
        vmaFreeMemory(vmaAllocator, m_allocations[inOutImage->allocationIndex].allocation);
        m_allocations[inOutImage->allocationIndex].freed = true;
        inOutImage->allocationIndex = UINT32_MAX;

        vkDestroyImage(logical, inOutImage->handle, nullptr);
        inOutImage->handle = VK_NULL_HANDLE;
    }

    auto Device::createImageView(Image const* pImage, ImageView* inOutView) -> bool
    {
        MXC_ASSERT( // redundant checks to make them fit in one line
            ((inOutView->viewType == VK_IMAGE_VIEW_TYPE_1D) && pImage->type == VK_IMAGE_TYPE_1D) ||
            ((inOutView->viewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY) && pImage->type == VK_IMAGE_TYPE_1D) ||
            ((inOutView->viewType == VK_IMAGE_VIEW_TYPE_2D) && pImage->type == VK_IMAGE_TYPE_2D) ||
            ((inOutView->viewType == VK_IMAGE_VIEW_TYPE_2D) && pImage->type == VK_IMAGE_TYPE_3D) ||
            ((inOutView->viewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY) && pImage->type == VK_IMAGE_TYPE_2D) ||
            ((inOutView->viewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY) && pImage->type == VK_IMAGE_TYPE_3D) ||
            ((inOutView->viewType == VK_IMAGE_VIEW_TYPE_CUBE) && pImage->type == VK_IMAGE_TYPE_2D) ||
            ((inOutView->viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) && pImage->type == VK_IMAGE_TYPE_2D) ||
            ((inOutView->viewType == VK_IMAGE_VIEW_TYPE_3D) && pImage->type == VK_IMAGE_TYPE_3D)
            , "Incompatible image view type and image type");

        VkImageViewCreateInfo const imageViewCreateInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = pImage->handle,
            .viewType = inOutView->viewType,
            .format = pImage->format,
            .components = { .r = VK_COMPONENT_SWIZZLE_IDENTITY, .g = VK_COMPONENT_SWIZZLE_IDENTITY, 
                            .b = VK_COMPONENT_SWIZZLE_IDENTITY, .a =  VK_COMPONENT_SWIZZLE_IDENTITY},
            .subresourceRange = {   .aspectMask = inOutView->aspectMask, 
                                    .baseMipLevel = inOutView->baseMipLevel, 
                                    .levelCount = inOutView->levelCount, 
                                    .baseArrayLayer = inOutView->baseArrayLayer, 
                                    .layerCount = inOutView->layerCount}
        };

        VK_CHECK(vkCreateImageView(logical, &imageViewCreateInfo, nullptr, &inOutView->handle));
        return true;
    }

    auto Device::destroyImageView(ImageView* pOutView) -> void
    {
        MXC_ASSERT(pOutView->isValid(), "Image view Given for destruction is not valid");
        MXC_DEBUG("Destroying one image view");
        vkDestroyImageView(logical, pOutView->handle, nullptr);
    }
    
    auto Device::insertImageMemoryBarrier(
        CommandBuffer* pCmdBuf,
        Image* pImage,
        VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout,
        VkImageAspectFlags imageAspectMask,
        VkPipelineStageFlags srcStageMask,
        VkPipelineStageFlags dstStageMask) -> void
    {
        VkImageSubresourceRange const subresourceRange {
            .aspectMask = imageAspectMask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        };
        insertImageMemoryBarrier(pCmdBuf, pImage, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
    }

    // Taken from Sascha Willelms examples
    auto Device::insertImageMemoryBarrier(
        CommandBuffer* pCmdBuf,
        Image* pImage,
        VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout,
        VkImageSubresourceRange subresourceRange,
        VkPipelineStageFlags srcStageMask,
        VkPipelineStageFlags dstStageMask) -> void
    {
        MXC_ASSERT(pCmdBuf && pImage && pCmdBuf->canRecord(), "Invalid arguments passed into Device::insertImageMemoryBarrier");
        // Create an image barrier object
        VkImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.oldLayout = oldImageLayout;
        imageMemoryBarrier.newLayout = newImageLayout;
        imageMemoryBarrier.image = pImage->handle;
        imageMemoryBarrier.subresourceRange = subresourceRange;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        // Source layouts (old)
        // Source access mask controls actions that have to be finished on the old layout
        // before it will be transitioned to the new layout
        switch (oldImageLayout)
        {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                // Image layout is undefined (or does not matter)
                // Only valid as initial layout
                // No flags required, listed only for completeness
                imageMemoryBarrier.srcAccessMask = 0;
                break;

            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                // Image is preinitialized
                // Only valid as initial layout for linear images, preserves memory contents
                // Make sure host writes have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image is a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image is a depth/stencil attachment
                // Make sure any writes to the depth/stencil buffer have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image is a transfer source
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image is a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image is read by a shader
                // Make sure any shader reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
        }

        // Target layouts (new)
        // Destination access mask controls the dependency for the new image layout
        switch (newImageLayout)
        {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image will be used as a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image will be used as a transfer source
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image will be used as a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image layout will be used as a depth/stencil attachment
                // Make sure any writes to depth/stencil buffer have been finished
                imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image will be read in a shader (sampler, input attachment)
                // Make sure any writes to the image have been finished
                if (imageMemoryBarrier.srcAccessMask == 0)
                {
                        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                }
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
        }

        // Put barrier inside setup command buffer
        vkCmdPipelineBarrier(
            pCmdBuf->handle,
            srcStageMask,
            dstStageMask,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);
    }

    constexpr auto chooseMemoryPropertyFlags(StorageUniformMemoryOptions options) -> VkMemoryPropertyFlags
    {
        switch (options)
        {
            case StorageUniformMemoryOptions::STAGING_AND_TRANSFER:      return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            case StorageUniformMemoryOptions::SYSTEM_MEMORY:             return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            case StorageUniformMemoryOptions::DEVICE_LOCAL_HOST_VISIBLE: return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }
    }
    
    // TODO: VmaPool
    // Note: maybe uniform and storages should be mapped by default if preferCPUMemory is passed
    constexpr auto chooseVmaAllocationCI(BufferType_t type, bool preferCPUmemory, bool GPUonlyResource) -> VmaAllocationCreateInfo
    {
        VmaAllocationCreateInfo res {};
        res.usage = VMA_MEMORY_USAGE_AUTO;

        if ((type & BufferType_v::VERTEX) == BufferType_v::VERTEX)
        {
            res.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        }
        else if ((type & BufferType_v::INDEX) == BufferType_v::INDEX)
        {
            res.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        }
        else if ((type & BufferType_v::UNIFORM) == BufferType_v::UNIFORM)
        {
            res.usage = preferCPUmemory ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            res.flags = GPUonlyResource ? VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT : (VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT);
        }
        else if ((type & BufferType_v::UNIFORM_TEXEL) == BufferType_v::UNIFORM_TEXEL)
        {
            res.usage = preferCPUmemory ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            res.flags = GPUonlyResource ? VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT : (VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT);
        }
        else if ((type & BufferType_v::STAGING) == BufferType_v::STAGING)
        {
            res.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        }

        // buffers can be read-write too, therefore no else if here
        if ((type & BufferType_v::STORAGE) == BufferType_v::STORAGE)
        {
            res.usage = preferCPUmemory ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            res.flags = GPUonlyResource ? VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT : (VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT);
        }
        else if ((type & BufferType_v::STORAGE_TEXEL) == BufferType_v::STORAGE_TEXEL)
        {
            res.usage = preferCPUmemory ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            res.flags = GPUonlyResource ? VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT : (VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT);
        }
        
        return res;
    }
}
