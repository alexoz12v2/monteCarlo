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
    
	auto CommandBuffer::allocateMany(VulkanContext* ctx, CommandBuffer* outCmdBuffers, uint32_t count) -> bool 
    {
        MXC_ASSERT(!any(outCmdBuffers, count, [](CommandBuffer const& c) -> bool { return c.m_state != State::NOT_ALLOCATED; }), 
                   "Cannot Allocate new Command Buffer without freeing existing one!");
        VkCommandBufferAllocateInfo const allocateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = ctx->device.commandPool, // TODO might refactor
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = count
        };

        std::vector<VkCommandBuffer> tempCmdBufs(count);
        VK_CHECK(vkAllocateCommandBuffers(ctx->device.logical, &allocateInfo, tempCmdBufs.data()));
        for (uint32_t i = 0; i != count; ++i)
        {
            outCmdBuffers[i].handle = tempCmdBufs[i];
            outCmdBuffers[i].m_state = State::INITIAL;
            outCmdBuffers[i].m_level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        }

        return true;
    }
    
    auto CommandBuffer::allocate(VulkanContext* ctx) -> bool 
    {
        MXC_ASSERT(m_state == State::NOT_ALLOCATED, "Cannot Allocate new Command Buffer without freeing existing one!");
        VkCommandBufferAllocateInfo const allocateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = ctx->device.commandPool, // TODO might refactor
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        VK_CHECK(vkAllocateCommandBuffers(ctx->device.logical, &allocateInfo, &handle));
        m_state = State::INITIAL;
        m_level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        return true;
    }

    auto CommandBuffer::free(VulkanContext* ctx) -> void
    {
        MXC_ASSERT(!(m_state == State::RECORDING || m_state == State::PENDING || m_state == State::NOT_ALLOCATED), 
                   "Cannot free a busy/not allocated Command Buffer!");
        vkFreeCommandBuffers(ctx->device.logical, ctx->device.commandPool, 1, &handle);
        m_state = State::NOT_ALLOCATED;
    }

    auto CommandBuffer::freeMany(VulkanContext* ctx, CommandBuffer* inOutCmdBuffers, uint32_t count) -> void 
    {
        MXC_ASSERT(!any(inOutCmdBuffers, count, 
                        [](CommandBuffer const& c)->bool
                        { return c.m_state == State::RECORDING || c.m_state == State::PENDING || c.m_state == State::NOT_ALLOCATED; })
                   , "Cannot free a busy/not allocated Command Buffer!");
        std::vector<VkCommandBuffer> tempCmdBufs(count);
        for (uint32_t i = 0; i != count; ++i)
        {
            tempCmdBufs[i] = inOutCmdBuffers[i].handle;
        }

        vkFreeCommandBuffers(ctx->device.logical, ctx->device.commandPool, count, tempCmdBufs.data());
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
        VK_CHECK(vkEndCommandBuffer(handle));
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
