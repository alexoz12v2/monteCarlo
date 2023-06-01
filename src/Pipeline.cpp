#include "Pipeline.h"
#include "CommandBuffer.h"
#include "Shader.h"
#include "VulkanContext.inl"
#include "logging.h"

#include <vector>

namespace mxc
{
    auto Pipeline::create(VulkanContext* ctx, ShaderSet const& shaderSet,uint32_t initialWidth, uint32_t initialHeight, VkRenderPass renderPass) -> bool
    {
        MXC_ASSERT(ctx, "Pipeline::create needs a valid VulkanContext!");

        // if there are no vertices and renderpass, then create a compute pipeline
        if (shaderSet.attributeDescriptions_count == 0 && renderPass == VK_NULL_HANDLE)
        {   
            MXC_ASSERT(shaderSet.stages.size() == 1, "compute shaders should be just a single shader");
            MXC_DEBUG("Pipeline::create trying to create a compute pipeline");

            bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
            ComputePipelineConfig const config {
                .descriptorSetLayouts = shaderSet.noResources ? nullptr : shaderSet.resources.descriptorSetLayouts.data(),
                .stage = shaderSet.stages[0],
                .descriptorSetLayoutCount = shaderSet.noResources ? 0 : static_cast<uint32_t>(shaderSet.resources.descriptorSetLayouts.size())
            };

            return create(ctx, config);
        }
        else // graphics pipeline
        {
            MXC_DEBUG("Pipeline::create trying to create a graphics pipeline");

            bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            VkViewport const viewport {
                .x = 0, .y = 0,
                .width = initialWidth, .height = initialHeight,
                .minDepth = 0, .maxDepth = 1
            };

            VkRect2D const scissor {
                .offset = {.x = 0, .y = 0},
                .extent = {.width = initialWidth, .height = initialHeight}
            };

            GraphicsPipelineConfig const config {
                .renderPass = renderPass,
                .attributes = shaderSet.attributeDescriptions,
                .descriptorSetLayouts = shaderSet.noResources ? nullptr : shaderSet.resources.descriptorSetLayouts.data(),
                .stages = shaderSet.stages.data(),
                .vertexBuffersBindingDescs = shaderSet.bindingDescriptions,
                .initialViewport = viewport,
                .initialScissor = scissor,
                .vertexBuffersCount = shaderSet.bindingDescriptions_count,
                .attributeCount = shaderSet.attributeDescriptions_count,
                .descriptorSetLayoutCount = shaderSet.noResources ? 0 : static_cast<uint32_t>(shaderSet.resources.descriptorSetLayouts.size()),
                .stageCount = static_cast<uint16_t>(shaderSet.stages.size()),
                .subpass = 0
            };

            return create(ctx, config);
        }
    }

    auto Pipeline::bind(CommandBuffer* pCmdBuf) -> void
    {
        MXC_ASSERT(pCmdBuf && pCmdBuf->canRecord(), "Pipeline::bind needs a valid, recording CommandBuffer!");
        vkCmdBindPipeline(pCmdBuf->handle, bindPoint, handle);
    }

    auto Pipeline::destroy(VulkanContext* ctx) -> void
    {
        MXC_ASSERT(ctx, "Pipeline::destroy needs a valid VulkanContext!");
        MXC_ASSERT(handle != VK_NULL_HANDLE, "pipeline is not in a valid constructed state");
        vkDestroyPipeline(ctx->device.logical, handle, nullptr);
        vkDestroyPipelineLayout(ctx->device.logical, layout, nullptr);
        vkDestroyPipelineCache(ctx->device.logical, cache, nullptr);
    }

    auto Pipeline::create(VulkanContext* ctx, GraphicsPipelineConfig const& config) -> bool
    {
        // vertex input state TODO configurable
        VkPipelineVertexInputStateCreateInfo vertexInputState{};
        vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputState.vertexBindingDescriptionCount = config.vertexBuffersCount;
        vertexInputState.pVertexBindingDescriptions = config.vertexBuffersBindingDescs;
        vertexInputState.vertexAttributeDescriptionCount = config.attributeCount;
        vertexInputState.pVertexAttributeDescriptions = config.attributes;

        MXC_ASSERT(vertexInputState.pVertexAttributeDescriptions[0].location == 0 &&
                   vertexInputState.pVertexAttributeDescriptions[1].location == 1, "what, they are %u and %u",
                   vertexInputState.pVertexAttributeDescriptions[0].location, vertexInputState.pVertexAttributeDescriptions[1].location);

        // input assembly state
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
        inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyState.primitiveRestartEnable = VK_FALSE;

        // tessellation state
        VkPipelineTessellationStateCreateInfo tessellationState{};
        tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        tessellationState.patchControlPoints = 1;
    
        // viewport state
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &config.initialViewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &config.initialScissor;

        // rasterization state
        VkPipelineRasterizationStateCreateInfo rasterizationState{};
        rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationState.depthClampEnable = VK_FALSE;
        rasterizationState.rasterizerDiscardEnable = VK_FALSE;
        rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationState.cullMode = VK_CULL_MODE_NONE;// VK_CULL_MODE_BACK_BIT;
        rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationState.lineWidth = 1.f;

        // multisample state
        VkPipelineMultisampleStateCreateInfo multisampleState{};
        multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // depth stencil state
        VkPipelineDepthStencilStateCreateInfo depthStencilState{};
        depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilState.depthTestEnable = VK_TRUE;
        depthStencilState.depthWriteEnable = VK_TRUE;
        depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    
        // color blend state
        // srcFactor * srcColor Op dstFactor * dstColor
        VkPipelineColorBlendAttachmentState const attachmentState {
            .blendEnable = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT

        };

        VkPipelineColorBlendStateCreateInfo colorBlendState{};
        colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendState.attachmentCount = 1; // Note: we got one color attachment
        colorBlendState.pAttachments = &attachmentState;

        // dynamic states
        static uint32_t constexpr dynamicStates_count = 2;
        VkDynamicState const dynamicStates[dynamicStates_count] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = dynamicStates_count;
        dynamicState.pDynamicStates = dynamicStates;

        createCacheAndLayout(ctx, config.descriptorSetLayouts, config.descriptorSetLayoutCount);

        // pipeline creation
        VkGraphicsPipelineCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stageCount = config.stageCount,
            .pStages = config.stages,
            .pVertexInputState = &vertexInputState,
            .pInputAssemblyState = &inputAssemblyState,
            .pTessellationState = &tessellationState,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizationState,
            .pMultisampleState = &multisampleState,
            .pDepthStencilState = &depthStencilState,
            .pColorBlendState = &colorBlendState,
            .pDynamicState = &dynamicState,
            .layout = layout,
            .renderPass = config.renderPass,
            .subpass = config.subpass,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = static_cast<uint32_t>(-1) // invalid index
        };
        VkResult res = vkCreateGraphicsPipelines(ctx->device.logical, cache, 1, &createInfo, nullptr, &handle);

#if defined(_DEBUG)
        if (res == VK_SUCCESS)
            MXC_DEBUG("Graphics pipeline created!");
#endif
        
        if (res != VK_SUCCESS)
        {
            MXC_ERROR("vkCreateGraphicsPipelines failed with %s", vulkanResultToString(res));
            return false;
        }

        return true;
    }

    auto Pipeline::create(VulkanContext* ctx, ComputePipelineConfig const& config) -> bool
    {
        createCacheAndLayout(ctx, config.descriptorSetLayouts, config.descriptorSetLayoutCount);
        
        VkComputePipelineCreateInfo const createInfo {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = config.stage,
            .layout = layout,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = static_cast<uint32_t>(-1)
        };

        VkResult res = vkCreateComputePipelines(ctx->device.logical, cache, 1, &createInfo, nullptr, &handle);

#if defined(_DEBUG)
        if (res == VK_SUCCESS)
            MXC_DEBUG("Compute pipeline created!");
#endif
        
        if (res != VK_SUCCESS)
        {
            MXC_ERROR("vkCreateComputePipelines failed with %s", vulkanResultToString(res));
            return false;
        }

        return true;
    }

    auto Pipeline::createCacheAndLayout(VulkanContext* ctx,VkDescriptorSetLayout const* descriptorSetLayouts, uint16_t descriptorSetLayoutCount) -> bool
    {
        // pipeline layout
        VkPipelineLayoutCreateInfo const layoutCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = descriptorSetLayoutCount,
            .pSetLayouts = descriptorSetLayouts,
            .pushConstantRangeCount = 0, // TODO push constants
            .pPushConstantRanges = nullptr,
        };
        VK_CHECK(vkCreatePipelineLayout(ctx->device.logical, &layoutCreateInfo, nullptr, &layout));

        // pipeline cache, TODO for future use
        VkPipelineCacheCreateInfo cacheCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .initialDataSize = 0,
            .pInitialData = nullptr
        };
        VK_CHECK(vkCreatePipelineCache(ctx->device.logical, &cacheCreateInfo, nullptr, &cache));
        return true;
    }
}
