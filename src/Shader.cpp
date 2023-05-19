#include "Shader.h"
#include "logging.h"

#include <dxc/dxcapi.h>

#include <array>
#include <string>

#define CHECK_DXC_RESULT(idxcblob) \
    do{HRESULT status;\
    idxcblob->GetStatus(&status);\
    MXC_ASSERT(status == S_OK, "Failed DXC library assertion");}while(0)

namespace mxc
{
    constexpr auto vkCopyDescriptorSet(VkDescriptorSet srcSet = VK_NULL_HANDLE, VkDescriptorSet dstSet = VK_NULL_HANDLE, uint8_t binding = UINT8_MAX, uint8_t arrayElement = 0, uint8_t count = 0) -> VkCopyDescriptorSet;
    auto compileShader(std::wstring filename) -> CComPtr<IDxcBlob>;

    auto ShaderResources::create(VulkanContext* ctx, ResourceConfiguration const& config, VkShaderStageFlagBits const* stageFlags) -> bool
    {
        // TODO: separate pool and set creation
        std::vector<VkDescriptorPoolSize> maxPoolSizes(config.poolSizes_count);
        for (uint32_t i = 0; i != maxPoolSizes.size(); ++i)
        {
            maxPoolSizes[i] = {config.pPoolSizes[i].type, MAX_DESCRIPTOR_COUNT_PER_TYPE};
        }

        VkDescriptorPoolCreateInfo const createInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .maxSets = MAX_DESCRIPTOR_SETS_COUNT,
            .poolSizeCount = config.poolSizes_count,
            .pPoolSizes = maxPoolSizes.data()
        };
        VK_CHECK(vkCreateDescriptorPool(ctx->device.logical, &createInfo, nullptr, &descriptorPool));

        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings(config.poolSizes_count); 
        for (uint32_t i = 0; i != setLayoutBindings.size(); ++i)
        {
            setLayoutBindings[i].binding = config.pBindingNumbers[i];
            setLayoutBindings[i].descriptorType = config.pPoolSizes[i].type;
            // This allows you to create arrays of descriptors and bind them to a single binding number.
            setLayoutBindings[i].descriptorCount = config.poolSizes_count; 
            setLayoutBindings[i].stageFlags = stageFlags[i];
            setLayoutBindings[i].pImmutableSamplers = nullptr;
        }

        VkDescriptorSetLayoutCreateInfo const layoutCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = static_cast<uint32_t>(setLayoutBindings.size()),
            .pBindings = setLayoutBindings.data(),
        };
        VK_CHECK(vkCreateDescriptorSetLayout(ctx->device.logical, &layoutCreateInfo, nullptr, &descriptorSetLayout));

        MXC_ASSERT(ctx->swapchain.images.size() <= 3, "More than three swapchain images. Not allowed");
        descriptorSets_count = static_cast<uint8_t>(ctx->swapchain.images.size());

        VkDescriptorSetLayout layouts[3] {descriptorSetLayout};
        
        VkDescriptorSetAllocateInfo allocateInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = descriptorPool,
            .descriptorSetCount = descriptorSets_count,
            .pSetLayouts = layouts
        };
        VK_CHECK(vkAllocateDescriptorSets(ctx->device.logical, &allocateInfo, descriptorSets));

        // copy all descriptor pool sizes in descriptor info
        m_descriptorsInfo.reserve(config.poolSizes_count);
        for (uint32_t i = 0; i != config.poolSizes_count; ++i)
        {
            m_descriptorsInfo.push_back({config.pPoolSizes[i].type, config.pPoolSizes[i].descriptorCount, config.pBindingNumbers[i]});
        }
 
        return true;
    }

    auto ShaderResources::destroy(VulkanContext* ctx) -> void
    {
        if (m_updateTemplateCreated) vkDestroyDescriptorUpdateTemplate(ctx->device.logical, descriptorUpdateTemplate, nullptr);
        vkFreeDescriptorSets(ctx->device.logical, descriptorPool, descriptorSets_count, descriptorSets);
        vkDestroyDescriptorSetLayout(ctx->device.logical, descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(ctx->device.logical, descriptorPool, nullptr);
    }

    auto ShaderSet::create(VulkanContext* ctx, ShaderConfiguration const& config, ResourceConfiguration const& resConfig) -> bool
    {
        stages.resize(config.stage_count);
        VkShaderModuleCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

        for (uint8_t i = 0; i != config.stage_count; ++i)
        {
            CComPtr<IDxcBlob> compiledShader = compileShader(config.filenames[i]);
            VkShaderModule shaderModule;

            createInfo.codeSize = static_cast<uint32_t>(compiledShader->GetBufferSize() / sizeof(uint32_t));
            createInfo.pCode = reinterpret_cast<uint32_t const*>(compiledShader->GetBufferPointer());
            VK_CHECK(vkCreateShaderModule(ctx->device.logical, &createInfo, nullptr, &shaderModule));

            MXC_ASSERT(config.stageFlags[i] == VK_SHADER_STAGE_VERTEX_BIT 
                       || config.stageFlags[i] == VK_SHADER_STAGE_FRAGMENT_BIT 
                       || config.stageFlags[i] == VK_SHADER_STAGE_COMPUTE_BIT
                       , "Provided Shader Stage not supported");
            stages[i].shaderStageCI = {
    		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    		.pNext = nullptr, // TODO maybe debug utils
    		.flags = 0,
    		.stage = config.stageFlags[i],
    		.module = shaderModule,
    		.pName = "main",
    		.pSpecializationInfo = nullptr // Note: might be useful in the future
            };
            
            resources.create(ctx, resConfig, config.stageFlags);
        }

        // save inputs to vertex shader
        attributeDescriptions_count = config.attributeDescriptions_count;
        for (uint32_t i = 0; i != attributeDescriptions_count; ++i)
        {
            attributeDescriptions[i] = config.attributeDescriptions[i];
        }
        return true;
    }

    auto ShaderSet::destroy(VulkanContext* ctx) -> void
    {
        resources.destroy(ctx);
        for (uint8_t i = 0; i != stages.size(); ++i)
        {
            vkDestroyShaderModule(ctx->device.logical, stages[i].shaderStageCI.module, nullptr);
        }
    }

    auto ShaderResources::createUpdateTemplate(VulkanContext* ctx, VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t const* strides) -> bool
    {
        std::vector<VkDescriptorUpdateTemplateEntry> descriptorUpdateTemplateEntries;
        descriptorUpdateTemplateEntries.reserve(m_descriptorsInfo.size());
        for (uint32_t i = 0; i != m_descriptorsInfo.size(); ++i)
        {
            descriptorUpdateTemplateEntries.push_back({
                .dstBinding = m_descriptorsInfo[i].bindingNumber,
                .dstArrayElement = 0,
                .descriptorCount = m_descriptorsInfo[i].descriptorCount,
                .descriptorType = m_descriptorsInfo[i].type,
                .offset = 0, // TODO might cause problems?
                .stride = strides[i]
            });
        }

        VkDescriptorUpdateTemplateCreateInfo const templateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .descriptorUpdateEntryCount = static_cast<uint32_t>(descriptorUpdateTemplateEntries.size()),
            .pDescriptorUpdateEntries = descriptorUpdateTemplateEntries.data(), // structs describing descriptors
            .templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET, // <- only update descriptor sets with fixed layouts. i.e. no push descriptors
            .descriptorSetLayout = descriptorSetLayout,
            .pipelineBindPoint = bindPoint,
            .pipelineLayout = layout,
            .set = 0// NUMBER of descriptor sets in the pipeline layout to be updated, ignored if templateType is not VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR
        };
        VK_CHECK(vkCreateDescriptorUpdateTemplate(ctx->device.logical, &templateCreateInfo, nullptr, &descriptorUpdateTemplate));
        m_updateTemplateCreated = true;
        
        return true;
    }

    auto ShaderResources::updateAll(VulkanContext* ctx, void const* data) -> bool
    {
        MXC_ASSERT(m_updateTemplateCreated, "Cannot Update Descriptor Set without a template");
        for (uint8_t i = 0; i != descriptorSets_count; ++i)
        {
            vkUpdateDescriptorSetWithTemplate(ctx->device.logical, descriptorSets[i], descriptorUpdateTemplate, data);
        }
        return true;
    }
    
    auto ShaderResources::copy(VulkanContext* ctx, uint8_t srcDescriptorSet, uint8_t dstDescriptorSet, uint8_t binding, uint8_t arrayElement, uint8_t count) -> void
    {
        VkCopyDescriptorSet const descriptorCopies[MAX_DESCRIPTOR_SETS_COUNT] {vkCopyDescriptorSet(
            descriptorSets[srcDescriptorSet], 
            descriptorSets[dstDescriptorSet], 
            binding, 
            arrayElement, 
            count)};
        vkUpdateDescriptorSets(ctx->device.logical, 0, nullptr, descriptorSets_count - 1, descriptorCopies);
    }
    
    // https://registry.khronos.org/vulkan/site/guide/latest/hlsl.html
    auto compileShader(std::wstring filename) -> CComPtr<IDxcBlob> // TODO remove string
    {
        // TODO RELEASE ALL MEMORY
        static bool compilerUninitialized = true;
        static CComPtr<IDxcLibrary> pLibrary{nullptr};
        static CComPtr<IDxcUtils> pUtils{nullptr};
        static CComPtr<IDxcCompiler3> pCompiler{nullptr};

        HRESULT hres;

        if (compilerUninitialized) [[unlikely]]
        {
            compilerUninitialized = false;

            // Initialize DXC library
            hres = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&pLibrary));
            MXC_ASSERT(!FAILED(hres), "Failed to initialize DXC library");

            // initialize DXC compiler
            hres = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler));
            MXC_ASSERT(!FAILED(hres), "Failed to create DXC compiler");

            // initialize DXC utility
            hres = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
            MXC_ASSERT(!FAILED(hres), "Failed to create Utilities for DXC library");
        }

        // Load HLSL shader from disk
        uint32_t codePage = DXC_CP_ACP; // ansi code page i.e. the default for the system
        CComPtr<IDxcBlobEncoding> pSourceBlob{nullptr};
        hres = pUtils->LoadFile(filename.c_str(), &codePage, &pSourceBlob);
        MXC_ASSERT(!FAILED(hres), "Failed to load file at %s", filename.c_str());

        // select shader target profile based on extension (basically the version of the shader language, model, type of shader)
        LPCWSTR targetProfile{};
        size_t idx = filename.rfind('.');
        if (idx != std::string::npos)
        {
            std::wstring extension = filename.substr(idx+1);
            if (extension == L"vert")
                targetProfile = L"vs_6_1";
            else if (extension == L"frag")
                targetProfile = L"ps_6_1";
            else if (extension == L"comp")
                targetProfile = L"cs_6_1";
            // TODO anything else
        }

        // compile shader
        std::array args { (LPCWSTR)
            filename.c_str(),               // optional filename to be displayed in case of compilation error
            L"-Zpc",                        // matrices in column-major order
            L"-HV", L"2021",                // HLSL version 2021
            L"-T", targetProfile,           // targetProfile       
            L"-E", L"main",                 // entryPoint TODO might expose as parameter
            L"-spirv",                      // compile to spirv
            L"-fspv-target-env=vulkan1.3"   // use vulkan1.3 environment
        };

        DxcBuffer srcBuffer {
            .Ptr = pSourceBlob->GetBufferPointer(),
            .Size = pSourceBlob->GetBufferSize(),
            .Encoding = DXC_CP_ACP // 
        };

        CComPtr<IDxcResult> pResult{nullptr};
        hres = pCompiler->Compile(
            &srcBuffer,         // DxcBuffer containing the shader source
            args.data(),        // arguments passed to the dxc compiler
            static_cast<uint32_t>(args.size()),
            nullptr,            // optional include handler to manage #includes in the shaders TODO 
            IID_PPV_ARGS(&pResult)
        );

        // Compilation failure handling
        if (FAILED(hres) && (pResult)) // checking result because the compile function might fail because of a compilation error or memory exhaustion
        {
            CComPtr<IDxcBlobEncoding> pErrorBlob;
            hres = pResult->GetErrorBuffer(&pErrorBlob);
            if (SUCCEEDED(hres) && pErrorBlob) [[likely]]
            {
                MXC_ERROR("Vertex Shader Compilation Error: %s", reinterpret_cast<char const*>(&pErrorBlob));
                assert(false);
            }
        }

        // get the compilation result
        CComPtr<IDxcBlob> spirvBinaryCode;
        pResult->GetResult(&spirvBinaryCode);
        MXC_TRACE("shader at %s compiled successfully", filename.c_str());
        return spirvBinaryCode;
    }

    // TODO force inline
    constexpr auto vkCopyDescriptorSet(VkDescriptorSet srcSet, VkDescriptorSet dstSet, uint8_t binding, uint8_t arrayElement, uint8_t count) -> VkCopyDescriptorSet
    {
        return {
            .sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
            .pNext = nullptr,
            .srcSet = srcSet,
            .srcBinding = binding,
            .srcArrayElement = arrayElement,
            .dstSet = dstSet,
            .dstBinding = binding,
            .dstArrayElement = arrayElement,
            .descriptorCount = count
        };
    }
}
