#ifndef MXC_COMMAND_BUFFER_H
#define MXC_COMMAND_BUFFER_H

#include "VulkanCommon.h"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace mxc 
{
	// interface inspired from https://github.com/travisvroman/kohi/tree/main
	class CommandBuffer
	{
	public:
		// TODO add overloads to the free and allocate functions which take device and pool
		static auto allocateMany(VulkanContext* ctx, CommandType const type, CommandBuffer* outCmdBuffers, uint32_t count) -> bool;
		auto allocate(VulkanContext* ctx, CommandType const type) -> bool;
		// assumes they have all the same type
		static auto freeMany(VulkanContext* ctx, CommandBuffer* inOutCmdBuffers, uint32_t count) -> void;
		auto free(VulkanContext* ctx) -> void;

		auto begin() -> bool;
		auto signalSubmit() -> bool;
		auto signalCompletion() -> bool;
		auto end() -> bool;

		auto reset() -> void; 

		auto canRecord() const -> bool { return m_state == State::RECORDING; }
		auto isExecutable() const -> bool { return m_state == State::EXECUTABLE; }
		auto isPrimary() const -> bool { return m_level == VK_COMMAND_BUFFER_LEVEL_PRIMARY; }

		VkCommandBuffer handle = VK_NULL_HANDLE;
	private:
		enum class State : uint8_t
		{
			NOT_ALLOCATED,	// instance just created, yet not allocated
			INITIAL,		// just allocated. Can either be freed or moved to recording
			RECORDING,		// vkBeginCommandBuffer has been just called. We can use vkCmd* stuff
			EXECUTABLE,		// vkEndCommandBuffer has been just called
			PENDING,		// executable command buffer submitted to a queue
			INVALID			// recording, executable, and pending, can be sent in this state (eg one time submission or bound resource deletion). reset
		};

		// Note: 1) there is just one resettable command pool now, stored in the Device class. modify this if you multithread this thing
		//		 2) now it is hard coded to create one command buffer per allocation
		VkCommandBufferLevel m_level; // for now hardcoded to primary
		CommandType m_type;
		State m_state = State::NOT_ALLOCATED;
	};

}

#endif // MXC_COMMAND_BUFFER_H
