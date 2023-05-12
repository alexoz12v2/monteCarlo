// most of the code is inspired from https://github.com/travisvroman/kohi/
#include "Swapchain.h"
#include "VulkanCommon.h"
#include "VulkanContext.inl"
#include "logging.h"


namespace mxc
{ 
    uint32_t clamp(uint32_t value, uint32_t min, uint32_t max)
    {
        return value < min ? min :
               value > max ? max :
               value;
    }

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    auto Swapchain::initSurface(VulkanContext* ctx, void* platformHandle, void* platformWindow) -> bool
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	 auto Swapchain::initSurface(VulkanContext* ctx, wl_display* display, wl_surface* window) -> bool
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    auto Swapchain::initSurface(VulkanContext* ctx, xcb_connection_t* connection, xcb_window_t window) -> bool
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
    auto Swapchain::initSurface(VulkanContext* ctx, void* view) -> bool
#endif
    {
        VkResult err;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.hinstance = reinterpret_cast<HINSTANCE>(platformHandle);
        surfaceCreateInfo.hwnd = reinterpret_cast<HWND>(platformWindow);

        err = vkCreateWin32SurfaceKHR(ctx->instance, &surfaceCreateInfo, nullptr, &ctx->surface);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo = {};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.display = display;
        surfaceCreateInfo.surface = window;

        err = vkCreateWaylandSurfaceKHR(ctx->instance, &surfaceCreateInfo, nullptr, &ctx->surface);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
        VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.connection = connection;
        surfaceCreateInfo.window = window;
        
        err = vkCreateXcbSurfaceKHR(ctx->instance, &surfaceCreateInfo, nullptr, &ctx->surface);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
        VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
        surfaceCreateInfo.sType = VK_STUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
        surfaceCreateInfo.pNext = nullptr;
        surfaceCreateInfo.flags = 0;
        surfaceCreateInfo.pView = view;
        
        err = vkCreateMacOSSurfaceMVK(ctx->instance, &surfaceCreateInfo, nullptr, &ctx->surface);
#endif
        if (err != VK_SUCCESS)
            return false;

        return true;
    }

    auto Swapchain::initFunctionPointers(VulkanContext* ctx) -> bool
    {
        fpGetPhysicalDeviceSurfaceSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(vkGetInstanceProcAddr(ctx->instance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
        fpGetPhysicalDeviceSurfaceCapabilitiesKHR =  reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(vkGetInstanceProcAddr(ctx->instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
        fpGetPhysicalDeviceSurfaceFormatsKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(vkGetInstanceProcAddr(ctx->instance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
        fpGetPhysicalDeviceSurfacePresentModesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(vkGetInstanceProcAddr(ctx->instance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));

        fpCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(vkGetDeviceProcAddr(ctx->device.logical, "vkCreateSwapchainKHR"));
        fpDestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(vkGetDeviceProcAddr(ctx->device.logical, "vkDestroySwapchainKHR"));
        fpGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(vkGetDeviceProcAddr(ctx->device.logical, "vkGetSwapchainImagesKHR"));
        fpAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(vkGetDeviceProcAddr(ctx->device.logical, "vkAcquireNextImageKHR"));
        fpQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(vkGetDeviceProcAddr(ctx->device.logical, "vkQueuePresentKHR"));
        return true;
    }

	auto Swapchain::create(VulkanContext* ctx, uint32_t width, uint32_t height, bool vsync) -> bool
    {
        VkExtent2D swapchainExtent = {width, height};

        // Choose a swap surface format.
        bool found = false;
        for (uint32_t i = 0; i < ctx->device.swapchainSupport.formats.size(); ++i) 
        {
            VkSurfaceFormatKHR format = ctx->device.swapchainSupport.formats[i];
            // Preferred formats
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
            {
                imageFormat = format;
                found = true;
                break;
            }
        }

        if (!found) {
            imageFormat = ctx->device.swapchainSupport.formats[0];
        }

        // FIFO and MAILBOX support vsync, IMMEDIATE does not.
        VkPresentModeKHR presentMode;
        bool presentModeFound = false;
        for (uint32_t i = 0; i < ctx->device.swapchainSupport.presentModes.size(); ++i) 
        {
            VkPresentModeKHR mode = ctx->device.swapchainSupport.presentModes[i];
            if (vsync && mode == VK_PRESENT_MODE_MAILBOX_KHR) 
            {
                presentMode = mode;
                break;
            }
            if (!vsync && mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                presentMode = mode;
                break;
            }
        }
            
        if (!presentModeFound)
            presentMode = VK_PRESENT_MODE_FIFO_KHR; // guaranteed by vulkan specs

        // Requery swapchain support.
        ctx->device.querySwapchainSupport(ctx, ctx->surface);

        // Swapchain extent
        if (ctx->device.swapchainSupport.surfCaps.currentExtent.width != UINT32_MAX) {
            swapchainExtent = ctx->device.swapchainSupport.surfCaps.currentExtent;
        }

        // Clamp to the value allowed by the GPU.
        VkExtent2D min = ctx->device.swapchainSupport.surfCaps.minImageExtent;
        VkExtent2D max = ctx->device.swapchainSupport.surfCaps.maxImageExtent;
        swapchainExtent.width = clamp(swapchainExtent.width, min.width, max.width);
        swapchainExtent.height = clamp(swapchainExtent.height, min.height, max.height);
        ctx->framebufferWidth = swapchainExtent.width;
        ctx->framebufferHeight = swapchainExtent.height;

        uint32_t imageCount = ctx->device.swapchainSupport.surfCaps.minImageCount + 1;
        if (ctx->device.swapchainSupport.surfCaps.maxImageCount > 0 && 
            imageCount > ctx->device.swapchainSupport.surfCaps.maxImageCount) 
        {
            imageCount = ctx->device.swapchainSupport.surfCaps.maxImageCount;
        }

        // setting max images simultaneously being rendered to max - 1 (the one being presented)
        maxFramesInFlight = imageCount - 1;

        // Swapchain create info
        VkSwapchainCreateInfoKHR swapchainCreateInfo{};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = ctx->surface;
        swapchainCreateInfo.minImageCount = imageCount;
        swapchainCreateInfo.imageFormat = imageFormat.format;
        swapchainCreateInfo.imageColorSpace = imageFormat.colorSpace;
        swapchainCreateInfo.imageExtent = swapchainExtent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // Setup the queue family indices
        if (ctx->device.queueFamilies.graphics!= ctx->device.queueFamilies.present) 
        {
            uint32_t queueFamilyIndices[] = {
                static_cast<uint32_t>(ctx->device.queueFamilies.graphics),
                static_cast<uint32_t>(ctx->device.queueFamilies.present)
            };
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainCreateInfo.queueFamilyIndexCount = 2;
            swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        } 
        else 
        {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapchainCreateInfo.queueFamilyIndexCount = 0;
            swapchainCreateInfo.pQueueFamilyIndices = 0;
        }

        swapchainCreateInfo.preTransform = ctx->device.swapchainSupport.surfCaps.currentTransform; // who cares about rotations
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = presentMode;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain = handle;

        VkSwapchainKHR oldSwapchain = handle;
        VK_CHECK(fpCreateSwapchainKHR(ctx->device.logical, &swapchainCreateInfo, nullptr, &handle));
        
        // destroy old swapchain
        for (uint32_t i = 0; i < images.size(); ++i)
        {
            vkDestroyImageView(ctx->device.logical, images[i].view, nullptr);
        }
        fpDestroySwapchainKHR(ctx->device.logical, oldSwapchain, nullptr);

        // Start with a zero frame index.
        currentImageIndex = 0;

        // Images
        uint32_t swapchainImageCount = 0;
        VK_CHECK(vkGetSwapchainImagesKHR(ctx->device.logical, handle, &swapchainImageCount, nullptr));
        VkImage swapchainImages[32];
        VK_CHECK(vkGetSwapchainImagesKHR(ctx->device.logical, handle, &swapchainImageCount, swapchainImages));

        images.resize(swapchainImageCount);
        for (uint32_t i = 0; i < swapchainImageCount; ++i) 
        {
            // Update the internal image for each.
            images[i].handle = swapchainImages[i];
        }

        // Views
        for (uint32_t i = 0; i < images.size(); ++i) 
        {
            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = images[i].handle;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = imageFormat.format;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            VK_CHECK(vkCreateImageView(ctx->device.logical, &viewInfo, nullptr, &images[i].view));
        }

        MXC_INFO("Swapchain created successfully.");
        return true;
    }

    auto Swapchain::destroy(VulkanContext* ctx) -> void
    {
        // view
        for (uint32_t i = 0; i != images.size(); ++i)
        {
            vkDestroyImageView(ctx->device.logical, images[i].view, nullptr);
            images[i].handle = VK_NULL_HANDLE;
        }

        // swapchain
        fpDestroySwapchainKHR(ctx->device.logical, handle, nullptr);
    }

    auto Swapchain::acquireNextImage(VulkanContext* ctx, VkSemaphore imageAvailableSemaphore, 
                                     uint32_t timeout_ns, VkFence signalFence, uint32_t* outIndex) -> bool
    {
        VkResult result = vkAcquireNextImageKHR(ctx->device.logical, handle, timeout_ns, imageAvailableSemaphore, signalFence, outIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            // Trigger swapchain recreation, then boot out of the render loop.
            ctx->resized();
            return false;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            MXC_ERROR("Failed to acquire swapchain image!");
            return false;
        }

        return true;
    }

    auto Swapchain::present(VulkanContext* ctx, VkSemaphore renderCompleteSemaphore)
    {
        // Return the image to the swapchain for presentation.
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderCompleteSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &handle;
        presentInfo.pImageIndices = &currentImageIndex;
        presentInfo.pResults = 0;

        VkResult result = vkQueuePresentKHR(ctx->device.presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            // Swapchain is out of date, suboptimal or a framebuffer resize has occurred. Trigger swapchain recreation.
            ctx->resized();
            MXC_DEBUG("Swapchain recreated because swapchain returned out of date or suboptimal.");
        } else if (result != VK_SUCCESS) {
            MXC_ERROR("Failed to present swap chain image!");
        }

        // Increment (and loop) the index.
        currentImageIndex = (currentImageIndex + 1) % maxFramesInFlight;
    }
    
}
