#ifndef MXC_PIPELINE_H
#define MXC_PIPELINE_H

#include <vulkan/vulkan.h>
#include "VulkanCommon.h"

#include <cstdint>

namespace mxc
{
    class CommandBuffer;
    class ShaderSet;
    
    // most of this data can be generated from createPipelineConfiguration with a ShaderSet
    struct GraphicsPipelineConfig
    {
        VkRenderPass renderPass;
        VkVertexInputAttributeDescription const* attributes;
        VkDescriptorSetLayout const* descriptorSetLayouts;
        VkPipelineShaderStageCreateInfo const* stages;
        VkVertexInputBindingDescription const* vertexBuffersBindingDescs;
        VkViewport initialViewport;
        VkRect2D initialScissor;
        uint16_t vertexBuffersCount;
        uint16_t attributeCount;
        uint16_t descriptorSetLayoutCount;
        uint16_t stageCount;
        uint16_t subpass;
    };

    struct ComputePipelineConfig
    {
        VkDescriptorSetLayout const* descriptorSetLayouts;
        VkPipelineShaderStageCreateInfo stage;
        uint16_t descriptorSetLayoutCount;
    };
    
	class Pipeline
	{
    public:
        // handles potential recreation due to events such as onResize
        auto create(VulkanContext* ctx, ShaderSet const& shaderSet, uint32_t initialWidth, uint32_t initialHeight, VkRenderPass renderPass = VK_NULL_HANDLE) -> bool;
        auto bind(CommandBuffer* pCmdBuf) -> void;

        auto destroy(VulkanContext* ctx) -> void;
        
        VkPipeline handle = VK_NULL_HANDLE;
        VkPipelineCache cache = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkPipelineBindPoint bindPoint;

    private:
        auto createCacheAndLayout(VulkanContext* ctx, VkDescriptorSetLayout const* descriptorSetLayouts, uint16_t descriptorSetLayoutCount) -> bool;
        auto create(VulkanContext* ctx, GraphicsPipelineConfig const& config) -> bool;
        auto create(VulkanContext* ctx, ComputePipelineConfig const& config) -> bool;
	};
}

#endif // MXC_PIPELINE_H
