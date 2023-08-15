#include "Shader.h"
#include "logging.h"

#include <dxc/dxcapi.h>

// TODO remove
#include <array>
#include <string>
#include <string_view>
#include <numeric>
#include <algorithm>
#include <unordered_set>
#include <filesystem>

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
    constexpr auto vkCopyDescriptorSet(VkDescriptorSet srcSet = VK_NULL_HANDLE, VkDescriptorSet dstSet = VK_NULL_HANDLE, 
                                       uint8_t binding = UINT8_MAX, uint8_t arrayElement = 0, uint8_t count = 0) -> VkCopyDescriptorSet;
    auto compileShader(std::wstring const& filename, std::wstring_view shaderDir) -> CComPtr<IDxcBlob>;
    constexpr auto VkDescriptorTypeToString(VkDescriptorType descriptorType) -> char const*;

    auto ShaderResources::create(VulkanContext* ctx, ResourceConfiguration const& config, VkShaderStageFlagBits const* stageFlags, 
                                 bool usePushDescriptors) -> bool
    {
        MXC_WARN("Current version of shaderResources puts all bindings, even bindings destined to different descriptor pools, in the same"
                 " descriptor set layout.");
        m_usePushDescriptors = usePushDescriptors;
        // Create Descriptor Pool ---------------------------------------------
        std::vector<VkDescriptorPoolSize> maxPoolSizes(config.poolSizes_count);
        for (uint32_t i = 0; i != maxPoolSizes.size(); ++i)
        {
            maxPoolSizes[i] = {config.pPoolSizes[i].type, MAX_DESCRIPTOR_SETS_COUNT*MAX_DESCRIPTOR_COUNT_PER_TYPE/*4 times 8*/};
        }

        VkDescriptorPoolCreateInfo const createInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .maxSets = MAX_DESCRIPTOR_SETS_COUNT/*8*/,
            .poolSizeCount = config.poolSizes_count,
            .pPoolSizes = maxPoolSizes.data()
        };
        VK_CHECK(vkCreateDescriptorPool(ctx->device.logical, &createInfo, nullptr, &descriptorPool));

        // Create Descriptor Set Layouts --------------------------------------
        VkDescriptorSetLayoutBinding setLayoutBinding[MAX_DESCRIPTOR_COUNT_PER_TYPE * MAX_DESCRIPTOR_SETS_COUNT];
        uint32_t runningOffset = 0;

        VkDescriptorSetLayoutCreateInfo const layoutCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = usePushDescriptors ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR : 0,
            .bindingCount = std::accumulate(config.pBindingNumbers_counts, config.pBindingNumbers_counts+config.poolSizes_count, 0), /*==2*/
            .pBindings = setLayoutBinding};
        MXC_ASSERT(layoutCreateInfo.bindingCount == 2, "What"); // TODO remove

        descriptorSetLayouts.resize(ctx->swapchain.images.size()); // TODO configurable?
        uint32_t index = 0;
        for (uint32_t i = 0; i!=1/*i != descriptorSetLayouts.size()*/; ++i) // TODO 
        {
            MXC_DEBUG("Creating Descriptor Set Layout with %u bindings", config.pBindingNumbers_counts[i]);
            for (uint32_t j = 0; j != 2/*config.pBindingNumbers_counts[i]*/; ++j)
            {
                setLayoutBinding[index].binding = config.pBindingNumbers[runningOffset + j];
                setLayoutBinding[index].descriptorType = config.pPoolSizes[i].type;
                // This allows you to create arrays of descriptors and bind them to a single binding number.
                setLayoutBinding[index].descriptorCount = 1; // descriptors per binding number
                setLayoutBinding[index].stageFlags = stageFlags[i];
                setLayoutBinding[index++].pImmutableSamplers = nullptr;
            }
            runningOffset += config.pBindingNumbers_counts[i];
        }

        VK_CHECK(vkCreateDescriptorSetLayout(ctx->device.logical, &layoutCreateInfo, nullptr, descriptorSetLayouts.data()));
        MXC_ASSERT(ctx->swapchain.images.size() <= 3, "More than three swapchain images. Not allowed");
        descriptorSets_count = static_cast<uint8_t>(ctx->swapchain.images.size());
        if (!usePushDescriptors)
        {
            std::fill(++descriptorSetLayouts.begin(), descriptorSetLayouts.end(), descriptorSetLayouts[0]);
            VkDescriptorSetAllocateInfo allocateInfo {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                .descriptorPool = descriptorPool,
                .descriptorSetCount = descriptorSets_count,
                .pSetLayouts = descriptorSetLayouts.data()
            };

            VK_CHECK(vkAllocateDescriptorSets(ctx->device.logical, &allocateInfo, descriptorSets));
        }

        // copy all descriptor pool sizes in descriptor metadata --------------
        runningOffset = 0;
        m_descriptorsMetadata.reserve(config.poolSizes_count);
        for (uint32_t i = 0; i != config.poolSizes_count; ++i)
        {
            MXC_DEBUG("descriptor type number: %u, %s", i, VkDescriptorTypeToString(config.pPoolSizes[i].type));
            // TODO refactor
            m_descriptorsMetadata.push_back({config.pPoolSizes[i].type, config.pPoolSizes[i].descriptorCount, {0}});
            for (uint32_t j = 0; j != config.pBindingNumbers_counts[i]; ++j)
            {
                MXC_DEBUG("\tbinding number: %u", j);
                m_descriptorsMetadata[i].bindingNumber[j] = config.pBindingNumbers[runningOffset + j];
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
            VkShaderModule shaderModule; // not necessary to store, as it is copied in the stages vector

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
                resources.create(ctx, resConfig, config.stageFlags, resConfig.usePushDescriptors); // TODO configurable bool 
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
                                               VkPipelineLayout layout, [[maybe_unused]]uint32_t const* strides) -> bool
    {
        std::vector<VkDescriptorUpdateTemplateEntry> descriptorUpdateTemplateEntries;
        descriptorUpdateTemplateEntries.reserve(m_descriptorsMetadata.size() * MAX_DESCRIPTOR_COUNT_PER_TYPE);

        uint32_t runningTotal = 0;
        for (uint32_t i = 0; i != m_descriptorsMetadata.size(); ++i) // for each descriptor type in a VkPoolSize
        {
            for (uint32_t j = 0; j != m_descriptorsMetadata[i].descriptorCount; ++j) // for each descriptor
            {
                MXC_WARN("update entry %u binding number: %u", i, m_descriptorsMetadata[i].bindingNumber[j]);
                descriptorUpdateTemplateEntries.push_back({
                    .dstBinding = m_descriptorsMetadata[i].bindingNumber[j],
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = m_descriptorsMetadata[i].type,
                    .offset = j * sizeof(VkDescriptorImageInfo), // TODO: make configurable
                    .stride = 0 // no stride since we are binding storage images for now. TODO: make configurable
                });
            }

            runningTotal += m_descriptorsMetadata[i].descriptorCount;
        }
        MXC_ASSERT(descriptorUpdateTemplateEntries.size()==2&&runningTotal==2,"in spectrum test there should be 2 descriptorUpdateTemplateEntries");

        // create a template for each set
        VkDescriptorUpdateTemplateCreateInfo templateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .descriptorUpdateEntryCount = runningTotal,
            .pDescriptorUpdateEntries = descriptorUpdateTemplateEntries.data(), // structs describing descriptors
            .templateType = m_usePushDescriptors ? VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR 
                                                 : VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET,
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
            templateCreateInfo.set = i;
            templateCreateInfo.descriptorSetLayout = descriptorSetLayouts[i];
            VK_CHECK(vkCreateDescriptorUpdateTemplate(ctx->device.logical, &templateCreateInfo, nullptr, &descriptorUpdateTemplates[i]));
            if (m_usePushDescriptors) break;
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
            vkUpdateDescriptorSetWithTemplate(ctx->device.logical, descriptorSets[i], descriptorUpdateTemplates[0], data);
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

    class CustomIncludeHandler : public IDxcIncludeHandler
    {
    public:
        CustomIncludeHandler(CComPtr<IDxcUtils> pUtils) : includedFiles(), pUtils(std::move(pUtils)) {}

        HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override
        {
            CComPtr<IDxcBlobEncoding> pEncoding;

            // Convert the Unicode filename to a multibyte filename using std::filesystem
            std::string multibyteFilename = std::filesystem::path(pFilename).string();
            
            // Normalize the file path using std::filesystem
            std::filesystem::path normalizedPath = std::filesystem::canonical(multibyteFilename);
    
            std::string path = normalizedPath.string();
            if (includedFiles.find(path) != includedFiles.end())
            {
                // Return empty string blob if this file has been included before
                static char const nullStr[] = " ";
                pUtils->CreateBlobFromPinned(nullStr, ARRAYSIZE(nullStr), DXC_CP_ACP, &pEncoding);
                *ppIncludeSource = pEncoding.Detach();
                return S_OK;
            }

            HRESULT hr = pUtils->LoadFile(pFilename, nullptr, &pEncoding);
            if (SUCCEEDED(hr))
            {
                includedFiles.insert(path);
                *ppIncludeSource = pEncoding.Detach();
            }
            return hr;
        }

        HRESULT STDMETHODCALLTYPE QueryInterface([[maybe_unused]]REFIID riid, 
                                                 [[maybe_unused]]_COM_Outptr_ void**/*__RPC_FAR* __RPC_FAR**/ ppvObject) override 
        { 
            return E_NOINTERFACE; 
        }
        ULONG STDMETHODCALLTYPE AddRef(void) override {	return 0; }
        ULONG STDMETHODCALLTYPE Release(void) override { return 0; }

        std::unordered_set<std::string> includedFiles;
        CComPtr<IDxcUtils> pUtils;
    };

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
        static CComPtr<CustomIncludeHandler> pIncludeHandler{nullptr};

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

            pIncludeHandler = new CustomIncludeHandler(pUtils);
            //hres = pUtils->CreateDefaultIncludeHandler(&pIncludeHandler);
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

    constexpr auto VkDescriptorTypeToString(VkDescriptorType descriptorType) -> char const*
    {
        switch (descriptorType) {
            case VK_DESCRIPTOR_TYPE_SAMPLER:
                return "VK_DESCRIPTOR_TYPE_SAMPLER";
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
            case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
            case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
            case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
            default:
                return "Unknown Descriptor Type";
        }
    }
}
