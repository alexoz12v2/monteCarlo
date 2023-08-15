#ifndef MXC_SHADER_H
#define MXC_SHADER_H

#include <vulkan/vulkan.h>
#include "VulkanCommon.h"
#include "VulkanContext.inl"

// TODO remove wstring
#include <cstdint>
#include <vector> // TODO refactor all vectors

namespace mxc
{

	struct ResourceConfiguration
	{
		VkDescriptorPoolSize const* pPoolSizes;
		uint32_t const* pBindingNumbers;
		uint32_t const* pBindingNumbers_counts;
		uint32_t poolSizes_count;
		bool usePushDescriptors;
	};
	
	class ShaderResources
	{
		static uint32_t constexpr MAX_DESCRIPTOR_COUNT_PER_TYPE = 4;
		static uint32_t constexpr MAX_DESCRIPTOR_SETS_COUNT = 8;
	public:
		// as many stageFlags as poolSizes_count
		auto create(VulkanContext* ctx, ResourceConfiguration const& config, VkShaderStageFlagBits const* stageFlags, bool usePushDescriptor) -> bool;
		auto destroy(VulkanContext* ctx) -> void;

		// stride is the size of each descriptor in bytes, returned from vkGet*MemoryRequirements. assumed to be equal to the count of descriptor sets
		auto createUpdateTemplate(VulkanContext* ctx, VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t const* strides) -> bool;
		auto updateAll(VulkanContext* ctx, void const* data) -> bool;
		auto update(VulkanContext* ctx, uint32_t descriptorIndex, void const* data) -> bool;
		auto copy(VulkanContext* ctx, uint8_t srcDescriptorSet, uint8_t dstDescriptorSet, uint8_t binding, uint8_t arrayElement, uint8_t count) -> void;

	public:
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE; 
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		VkDescriptorSet descriptorSets[MAX_DESCRIPTOR_SETS_COUNT]; // as many as framebuffers/command buffers
		std::vector<VkDescriptorUpdateTemplate> descriptorUpdateTemplates;
		uint8_t descriptorSets_count = 0;

	private:
		struct DescriptorMetadata
		{
			VkDescriptorType type;
			uint32_t descriptorCount;
			uint32_t bindingNumber[MAX_DESCRIPTOR_COUNT_PER_TYPE];
		};
		std::vector<DescriptorMetadata> m_descriptorsMetadata;
		bool m_updateTemplateCreated = false;
		bool m_usePushDescriptors;
	};
	
	struct ShaderConfiguration
	{
		wchar_t const** filenames; 
		VkShaderStageFlagBits const* stageFlags;
		wchar_t const* shaderDir;
		VkVertexInputAttributeDescription const* attributeDescriptions; 
		VkVertexInputBindingDescription const* bindingDescriptions;
		uint32_t bindingDescriptions_count;
		uint32_t attributeDescriptions_count;
		uint32_t stage_count;
	};

	// Note TODO Maybe add support for storage of more shader resources, and then choose one? Or sets with different layouts?
	// TODO refactor resorces and vertices out of this class
	class ShaderSet
	{
		static uint32_t constexpr MAX_SHADER_ATTRIBUTES = 16;
	public:
		auto create(VulkanContext* ctx, ShaderConfiguration const& config, ResourceConfiguration const& resConfig) -> bool;
		auto destroy(VulkanContext* ctx) -> void;

		std::vector<VkPipelineShaderStageCreateInfo> stages;

		ShaderResources resources;
		VkVertexInputAttributeDescription attributeDescriptions[MAX_SHADER_ATTRIBUTES];
		VkVertexInputBindingDescription bindingDescriptions[MAX_SHADER_ATTRIBUTES];
		uint32_t attributeDescriptions_count = 0;
		uint32_t bindingDescriptions_count = 0;
		bool noResources = false;
	};
}

#endif // MXC_SHADER_H

// put in implementation file
