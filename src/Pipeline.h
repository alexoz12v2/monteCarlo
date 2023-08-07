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
	VkPushConstantRange const* pPushConstantRanges;
        VkViewport initialViewport;
        VkRect2D initialScissor;
	uint32_t pushConstantRangesCount;
        uint16_t vertexBuffersCount;
        uint16_t attributeCount;
        uint16_t descriptorSetLayoutCount;
        uint16_t stageCount;
        uint16_t subpass;
    };

    struct ComputePipelineConfig
    {
        VkDescriptorSetLayout const* descriptorSetLayouts;
	VkPushConstantRange const* pPushConstantRanges;
        VkPipelineShaderStageCreateInfo stage;
	uint32_t pushConstantRangesCount;
        uint16_t descriptorSetLayoutCount;
    };
    
    class Pipeline
    {
    public:
        // handles potential recreation due to events such as onResize
        auto create(VulkanContext* ctx, ShaderSet const& shaderSet, uint32_t initialWidth, uint32_t initialHeight, VkRenderPass renderPass = VK_NULL_HANDLE, VkPushConstantRange const* pPushConstantRanges = nullptr, uint32_t pushConstantRanges_count = 0) -> bool;
        auto bind(CommandBuffer* pCmdBuf) -> void;

        auto destroy(VulkanContext* ctx) -> void;
        
        VkPipeline handle = VK_NULL_HANDLE;
        VkPipelineCache cache = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkPipelineBindPoint bindPoint;

    private:
        auto createCacheAndLayout(VulkanContext* ctx, VkDescriptorSetLayout const* descriptorSetLayouts, uint16_t descriptorSetLayoutCount,
                                  uint32_t pushConstantRangesCount, VkPushConstantRange const* pPushConstantRanges) -> bool;
        auto create(VulkanContext* ctx, GraphicsPipelineConfig const& config) -> bool;
        auto create(VulkanContext* ctx, ComputePipelineConfig const& config) -> bool;
    };
}

#endif // MXC_PIPELINE_H
