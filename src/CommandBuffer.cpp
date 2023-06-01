#include "CommandBuffer.h"


#include "logging.h"
#include "VulkanContext.inl"

#include <cassert>
#include <vector>

namespace mxc
{
    // instance of reduce, which in C++ is called accumulate. Note to self: avoiding to include algorithm
    constexpr auto any(CommandBuffer* range, uint32_t count, bool (*func)(CommandBuffer const&)) -> bool 
    {
        for (uint32_t i = 0; i != count; ++i)
        {
            if (func(range[i]) == true) 
            {
                return true;
            }
        }
        return false;
    }
    
    constexpr auto forEach(CommandBuffer* range, uint32_t count, void (*func)(CommandBuffer&)) -> void
    {
        for (uint32_t i = 0; i != count; ++i)
        {
            func(range[i]);
        }
    }
    
	auto CommandBuffer::allocateMany(VulkanContext* ctx, CommandType const type, CommandBuffer* outCmdBuffers, uint32_t count) -> bool 
    {
        MXC_ASSERT(!any(outCmdBuffers, count, [](CommandBuffer const& c) -> bool { return c.m_state != State::NOT_ALLOCATED; }), 
                   "Cannot Allocate new Command Buffer without freeing existing one!");
        VkCommandBufferAllocateInfo allocateInfo {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.pNext = nullptr;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = count;

        switch (type)
        {
            case CommandType::GRAPHICS: allocateInfo.commandPool = ctx->device.graphicsCmdPool; break;
            case CommandType::TRANSFER: allocateInfo.commandPool = ctx->device.transferCmdPool; break;
            case CommandType::COMPUTE:  allocateInfo.commandPool = ctx->device.computeCmdPool; break;
        }

        std::vector<VkCommandBuffer> tempCmdBufs(count);
        VK_CHECK(vkAllocateCommandBuffers(ctx->device.logical, &allocateInfo, tempCmdBufs.data()));
        for (uint32_t i = 0; i != count; ++i)
        {
            outCmdBuffers[i].handle = tempCmdBufs[i];
            outCmdBuffers[i].m_state = State::INITIAL;
            outCmdBuffers[i].m_level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            outCmdBuffers[i].m_type = type;
        }

        return true;
    }
    
    auto CommandBuffer::allocate(VulkanContext* ctx, CommandType const type) -> bool 
    {
        MXC_ASSERT(m_state == State::NOT_ALLOCATED, "Cannot Allocate new Command Buffer without freeing existing one!");
        VkCommandBufferAllocateInfo allocateInfo {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.pNext = nullptr;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;

        m_type = type;
        switch (type)
        {
            case CommandType::GRAPHICS: allocateInfo.commandPool = ctx->device.graphicsCmdPool; break;
            case CommandType::TRANSFER: allocateInfo.commandPool = ctx->device.transferCmdPool; break;
            case CommandType::COMPUTE:  allocateInfo.commandPool = ctx->device.computeCmdPool; break;
        }

        VK_CHECK(vkAllocateCommandBuffers(ctx->device.logical, &allocateInfo, &handle));
        m_state = State::INITIAL;
        m_level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        return true;
    }

    auto CommandBuffer::free(VulkanContext* ctx) -> void
    {
        // TODO remove and put queue idle based on type of command buffer
        reset();
        VK_CHECK(vkDeviceWaitIdle(ctx->device.logical));
        VkCommandPool cmdPool = m_type == CommandType::GRAPHICS ? ctx->device.graphicsCmdPool :
                                m_type == CommandType::TRANSFER ? ctx->device.transferCmdPool :
                                m_type == CommandType::COMPUTE  ? ctx->device.computeCmdPool  :
                                VK_NULL_HANDLE;
        vkFreeCommandBuffers(ctx->device.logical, cmdPool, 1, &handle);
        m_state = State::NOT_ALLOCATED;
    }

    // TODO add check such that all types are equal, and if they are not either crash or fallback to free throwing a warning
    auto CommandBuffer::freeMany(VulkanContext* ctx, CommandBuffer* inOutCmdBuffers, uint32_t count) -> void 
    {
        VK_CHECK(vkDeviceWaitIdle(ctx->device.logical));
        std::vector<VkCommandBuffer> tempCmdBufs(count);
        for (uint32_t i = 0; i != count; ++i)
        {
            inOutCmdBuffers[i].reset();
            tempCmdBufs[i] = inOutCmdBuffers[i].handle;
        }
        VkCommandPool cmdPool = inOutCmdBuffers[0].m_type == CommandType::GRAPHICS ? ctx->device.graphicsCmdPool :
                                inOutCmdBuffers[0].m_type == CommandType::TRANSFER ? ctx->device.transferCmdPool :
                                inOutCmdBuffers[0].m_type == CommandType::COMPUTE  ? ctx->device.computeCmdPool  :
                                VK_NULL_HANDLE;

        vkFreeCommandBuffers(ctx->device.logical, cmdPool, count, tempCmdBufs.data());
        for (uint32_t i = 0; i != count; ++i)
        {
            inOutCmdBuffers[i].handle = VK_NULL_HANDLE;
            inOutCmdBuffers[i].m_state = State::NOT_ALLOCATED;
        }
    }

    auto CommandBuffer::begin() -> bool 
    {
        MXC_ASSERT(!(m_state == State::RECORDING || m_state == State::PENDING), 
                   "Cannot begin recoring to a CommandBuffer Already recorded or submitted");
        static VkCommandBufferBeginInfo const beginInfo { // TODO configurable
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pInheritanceInfo = nullptr
        };
        VK_CHECK(vkBeginCommandBuffer(handle, &beginInfo));
        m_state = State::RECORDING;
        return true;
    }

    auto CommandBuffer::signalSubmit() -> bool 
    {
        MXC_ASSERT(m_state == State::EXECUTABLE, "Cannot submit a non-executable Command Buffer");
        m_state = State::PENDING;
        return true;
    }
    
    auto CommandBuffer::signalCompletion() -> bool
    {
        MXC_ASSERT(m_state == State::PENDING, "Cannot complete a non-pending Command Buffer");
        m_state = State::EXECUTABLE; // TODO: if pool is one time submit then this should be invalid
        return true;
    }

    auto CommandBuffer::end() -> bool 
    {
        MXC_ASSERT(m_state == State::RECORDING, "Cannot end recording of a non-recording command buffer!");
        if (VK_SUCCESS != vkEndCommandBuffer(handle))
        {   
            MXC_ERROR("Couldn't end the command buffer");
            return false;
        }
        m_state = State::EXECUTABLE;
        return true;
    }

    auto CommandBuffer::reset() -> void 
    {   
        MXC_ASSERT(m_state != State::PENDING, "Cannot reset a pending Command Buffer");
        VK_CHECK(vkResetCommandBuffer(handle, 0));
        m_state = State::INITIAL;
    } 
}
