#include "Shader.h"
#include "logging.h"

#include <dxc/dxcapi.h>

// TODO remove
#include <array>
#include <string>
#include <string_view>
#include <numeric>
#include <algorithm>

// TODO support for wchar_t logging
#if defined(_DEBUG)
#include <iostream>
#endif

#define CHECK_DXC_RESULT(idxcblob) \
    do{HRESULT status;\
    idxcblob->GetStatus(&status);\
    MXC_ASSERT(status == S_OK, "Failed DXC library assertion");}while(0)

namespace mxc
{
    constexpr auto vkCopyDescriptorSet(VkDescriptorSet srcSet = VK_NULL_HANDLE, VkDescriptorSet dstSet = VK_NULL_HANDLE, uint8_t binding = UINT8_MAX, uint8_t arrayElement = 0, uint8_t count = 0) -> VkCopyDescriptorSet;
    auto compileShader(std::wstring const& filename, std::wstring_view shaderDir) -> CComPtr<IDxcBlob>;

    // TODO FIX IT IMMEDIATELY. BINDING NUMBERS SIZE != POOLSIZES COUNT
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

        VkDescriptorSetLayoutBinding setLayoutBinding[MAX_DESCRIPTOR_COUNT_PER_TYPE * MAX_DESCRIPTOR_SETS_COUNT];
        uint32_t runningOffset = 0;

        VkDescriptorSetLayoutCreateInfo layoutCreateInfo {};
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
        layoutCreateInfo.bindingCount = std::accumulate(config.pBindingNumbers_counts, config.pBindingNumbers_counts+config.poolSizes_count, 0);
        layoutCreateInfo.pBindings = setLayoutBinding;

        descriptorSetLayouts.resize(1); // TODO configurable?
        uint32_t index = 0;
        for (uint32_t i = 0; i != descriptorSetLayouts.size(); ++i)
        {
            MXC_DEBUG("Creating Descriptor Set Layout with %u bindings", config.pBindingNumbers_counts[i]);
            for (uint32_t j = 0; j != config.pBindingNumbers_counts[i]; ++j)
            {
                setLayoutBinding[index].binding = config.pBindingNumbers[runningOffset + j];
                setLayoutBinding[index].descriptorType = config.pPoolSizes[i].type;
                // This allows you to create arrays of descriptors and bind them to a single binding number.
                setLayoutBinding[index].descriptorCount = 1; // descriptors per binding number
                setLayoutBinding[index].stageFlags = stageFlags[i];
                setLayoutBinding[index++].pImmutableSamplers = nullptr;
            }
        }

        VK_CHECK(vkCreateDescriptorSetLayout(ctx->device.logical, &layoutCreateInfo, nullptr, descriptorSetLayouts.data()));

        MXC_ASSERT(ctx->swapchain.images.size() <= 3, "More than three swapchain images. Not allowed");
        descriptorSets_count = static_cast<uint8_t>(ctx->swapchain.images.size());

        [[maybe_unused]] VkDescriptorSetAllocateInfo allocateInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = descriptorPool,
            .descriptorSetCount = descriptorSets_count,
            .pSetLayouts = descriptorSetLayouts.data() 
        };
        // TODO: clean up interface so that you can choose to use either regular desctiptor sets or push desctiptor sets
        // VK_CHECK(vkAllocateDescriptorSets(ctx->device.logical, &allocateInfo, descriptorSets));

        // copy all descriptor pool sizes in descriptor info
        runningOffset = 0;
        m_descriptorsInfo.reserve(config.poolSizes_count);
        for (uint32_t i = 0; i != config.poolSizes_count; ++i)
        {
            MXC_DEBUG("descriptor type number: %u", i);
            // TODO refactor
            m_descriptorsInfo.push_back({config.pPoolSizes[i].type, config.pPoolSizes[i].descriptorCount, {0}});
            for (uint32_t j = 0; j != config.pBindingNumbers_counts[i]; ++j)
            {
                MXC_DEBUG("binding number: %u", j);
                m_descriptorsInfo[i].bindingNumber[j] = config.pBindingNumbers[runningOffset + j];
            }
            runningOffset += config.pBindingNumbers_counts[i];
        }
 
        return true;
    }

    auto ShaderResources::destroy(VulkanContext* ctx) -> void
    {
        if (m_updateTemplateCreated) 
            for (auto dTemplate : descriptorUpdateTemplates)
                vkDestroyDescriptorUpdateTemplate(ctx->device.logical, dTemplate, nullptr);
        // TODO remember to uncomment this when working with regular descriptors
        //vkFreeDescriptorSets(ctx->device.logical, descriptorPool, descriptorSets_count, descriptorSets);
        for (auto layout : descriptorSetLayouts)
            vkDestroyDescriptorSetLayout(ctx->device.logical, layout, nullptr);

        vkDestroyDescriptorPool(ctx->device.logical, descriptorPool, nullptr);
    }

    auto ShaderSet::create(VulkanContext* ctx, ShaderConfiguration const& config, ResourceConfiguration const& resConfig) -> bool
    {
        stages.resize(config.stage_count);
        VkShaderModuleCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

        for (uint8_t i = 0; i != config.stage_count; ++i)
        {
            CComPtr<IDxcBlob> compiledShader = compileShader(config.filenames[i], config.shaderDir);
            VkShaderModule shaderModule;

            createInfo.codeSize = static_cast<uint32_t>(compiledShader->GetBufferSize()); // code size IN BYTES
            createInfo.pCode = reinterpret_cast<uint32_t const*>(compiledShader->GetBufferPointer());
            VK_CHECK(vkCreateShaderModule(ctx->device.logical, &createInfo, nullptr, &shaderModule));

            MXC_ASSERT(config.stageFlags[i] == VK_SHADER_STAGE_VERTEX_BIT 
                       || config.stageFlags[i] == VK_SHADER_STAGE_FRAGMENT_BIT 
                       || config.stageFlags[i] == VK_SHADER_STAGE_COMPUTE_BIT
                       , "Provided Shader Stage not supported");
            stages[i] = {
    		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    		.pNext = nullptr, // TODO maybe debug utils
    		.flags = 0,
    		.stage = config.stageFlags[i],
    		.module = shaderModule,
    		.pName = "main",
    		.pSpecializationInfo = nullptr // Note: might be useful in the future
            };
            
            if (resConfig.poolSizes_count != 0)
            {
                #if defined(_DEBUG)
                if (!resConfig.pPoolSizes)
                    MXC_WARN("creating shader resources with a poolSizes count > 0 but pPoolSizes == nullptr");
                #endif
                resources.create(ctx, resConfig, config.stageFlags);
                noResources = false;
            }
            else
                noResources = true;
        }

        // save inputs to vertex shader
        if (config.attributeDescriptions_count != 0)
        {
            attributeDescriptions_count = config.attributeDescriptions_count;
            bindingDescriptions_count = config.bindingDescriptions_count;
            for (uint32_t i = 0; i != attributeDescriptions_count; ++i)
            {
                attributeDescriptions[i] = config.attributeDescriptions[i];
            }
            for (uint32_t i = 0; i != bindingDescriptions_count; ++i)
            {
                bindingDescriptions[i] = config.bindingDescriptions[i];
            }
        }
        return true;
    }

    auto ShaderSet::destroy(VulkanContext* ctx) -> void
    {
        if (!noResources)
            resources.destroy(ctx);
        for (uint8_t i = 0; i != stages.size(); ++i)
        {
            vkDestroyShaderModule(ctx->device.logical, stages[i].module, nullptr);
        }
    }

    auto ShaderResources::createUpdateTemplate(VulkanContext* ctx, VkPipelineBindPoint bindPoint, 
                                               VkPipelineLayout layout, uint32_t const* strides) -> bool
    {
        std::vector<VkDescriptorUpdateTemplateEntry> descriptorUpdateTemplateEntries;
        descriptorUpdateTemplateEntries.reserve(m_descriptorsInfo.size() * MAX_DESCRIPTOR_COUNT_PER_TYPE);
        //MXC_ASSERT(descriptorUpdateTemplateEntries.capacity() == 4, "dfs");
        uint32_t runningTotal = 0;
        for (uint32_t i = 0; i != m_descriptorsInfo.size(); ++i)
        {
            for (uint32_t j = 0; j != m_descriptorsInfo[i].descriptorCount; ++j)
            {
                MXC_WARN("update entry %u binding number: %u", i, m_descriptorsInfo[i].bindingNumber[j]);
                descriptorUpdateTemplateEntries.push_back({
                    .dstBinding = m_descriptorsInfo[i].bindingNumber[j],
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = m_descriptorsInfo[i].type,
                    .offset = 0,
                    .stride = strides[i]
                });
            }

            runningTotal += m_descriptorsInfo[i].descriptorCount;
        }

        //MXC_ASSERT(runningTotal == 2, "fdsafsadfsd");

        // create a template for each set
        VkDescriptorUpdateTemplateCreateInfo templateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .descriptorUpdateEntryCount = runningTotal,
            .pDescriptorUpdateEntries = descriptorUpdateTemplateEntries.data(), // structs describing descriptors
            .templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR,
            .descriptorSetLayout = descriptorSetLayouts[0],
            .pipelineBindPoint = bindPoint,
            .pipelineLayout = layout,
            // set number of descriptor set in the pipeline layout to be updated, ignored if templateType is not 
            // VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR
            .set = 0
        };
        descriptorUpdateTemplates.resize(descriptorSets_count);
        // TODO: when using push descriptor sets, 1 layout == one push descriptor set. Handle case in which regular descriptor sets are created
        for (uint32_t i = 0; i != descriptorSets_count; ++i) 
        {
            // TODO refactor
            templateCreateInfo.set = 0;//i;
            templateCreateInfo.descriptorSetLayout = descriptorSetLayouts[i];
            VK_CHECK(vkCreateDescriptorUpdateTemplate(ctx->device.logical, &templateCreateInfo, nullptr, &descriptorUpdateTemplates[i]));
        }
        m_updateTemplateCreated = true;
        
        return true;
    }

    // pData is a pointer to memory containing one or more VkDescriptorImageInfo, VkDescriptorBufferInfo, or VkBufferView 
    // structures or VkAccelerationStructureKHR or VkAccelerationStructureNV handles used to write the descriptors.
    auto ShaderResources::updateAll(VulkanContext* ctx, void const* data) -> bool
    {
        MXC_ASSERT(m_updateTemplateCreated, "Cannot Update Descriptor Set without a template");
        for (uint8_t i = 0; i != descriptorSets_count; ++i)
        {
            vkUpdateDescriptorSetWithTemplate(ctx->device.logical, descriptorSets[i], descriptorUpdateTemplates[i], data);
        }
        return true;
    }

    auto ShaderResources::update(VulkanContext* ctx, uint32_t descriptorIndex, void const* data) -> bool
    {
        MXC_DEBUG("updating descriptor %u", descriptorIndex);
        MXC_ASSERT(m_updateTemplateCreated, "Cannot Update Descriptor Set without a template");
        vkUpdateDescriptorSetWithTemplate(ctx->device.logical, descriptorSets[descriptorIndex], descriptorUpdateTemplates[descriptorIndex], data);
        return true;
    }
    
    auto ShaderResources::copy(VulkanContext* ctx, uint8_t srcDescriptorSet, uint8_t dstDescriptorSet, 
                               uint8_t binding, uint8_t arrayElement, uint8_t count) -> void
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
    auto compileShader(std::wstring const& filename, std::wstring_view shaderDir) -> CComPtr<IDxcBlob> // TODO remove string
    {
    #if defined(_DEBUG)
        std::wcout << __FILE__ << L' ' << __LINE__ << L" [TRACE]: " << "filename of shader to compile = " << filename << L'\n';
    #endif

        static bool compilerUninitialized = true;
        static CComPtr<IDxcLibrary> pLibrary{nullptr};
        static CComPtr<IDxcUtils> pUtils{nullptr};
        static CComPtr<IDxcCompiler3> pCompiler{nullptr};
        static CComPtr<IDxcIncludeHandler> pIncludeHandler{nullptr};

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

            hres = pUtils->CreateDefaultIncludeHandler(&pIncludeHandler);
            MXC_ASSERT(!FAILED(hres), "Failed to create Include Handler for DXC library");
        }

        // Load HLSL shader from disk
        uint32_t codePage = DXC_CP_ACP; // ansi code page i.e. the default for the system
        CComPtr<IDxcBlobEncoding> pSourceBlob{nullptr};
        hres = pUtils->LoadFile(filename.c_str(), &codePage, &pSourceBlob);
        MXC_ASSERT(!FAILED(hres), "Failed to load file");

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
            filename.data(),                // optional filename to be displayed in case of compilation error
            L"-Zpc",                        // matrices in column-major order
            L"-HV", L"2021",                // HLSL version 2021
            L"-T", targetProfile,           // targetProfile       
            L"-E", L"main",                 // entryPoint TODO might expose as parameter
            L"-spirv",                      // compile to spirv
            L"-fspv-target-env=vulkan1.3",  // use vulkan1.3 environment
            L"-I", shaderDir.data()         // Shader Include Directories
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
            pIncludeHandler.p,            // optional include handler to manage #includes in the shaders TODO 
            IID_PPV_ARGS(&pResult)
        );

        // Compilation failure handling
        CComPtr<IDxcBlobUtf8> pErrors{nullptr};
        pResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
        if (pErrors && pErrors->GetStringLength() > 0)
        {
            MXC_ERROR(pErrors->GetStringPointer());
            std::ranges::for_each(args, [](auto const& arg) { 
                std::wcout << L"\033[31m" << __FILE__ << L" " << __LINE__ << L" [ERROR]: Argument passed = " << arg << L'\n';
            });
            assert(false);
        }
        MXC_TRACE("compilation passed");

        // get the compilation result
        CComPtr<IDxcBlob> spirvBinaryCode;
        pResult->GetResult(&spirvBinaryCode);
        return spirvBinaryCode;
    }

    // TODO force inline
    constexpr auto vkCopyDescriptorSet(VkDescriptorSet srcSet, VkDescriptorSet dstSet, uint8_t binding, 
                                       uint8_t arrayElement, uint8_t count) -> VkCopyDescriptorSet
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
