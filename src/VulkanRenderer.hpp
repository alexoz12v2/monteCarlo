#ifndef MXC_VULKAN_RENDERER_HPP
#define MXC_VULKAN_RENDERER_HPP

#include "StaticVector.hpp"
#include "initializers.hpp"
#include "utils.hpp"
#include "Scene.hpp"
#include "GPUTask.hpp"

#include <Eigen/Core>
#include <vulkan/vulkan.h>

#include <span>
#include <string_view>
#include <memory>

// TODO replace std::vector or add allocator to it
namespace mxc
{

// ------------------- Vulkan Renderer Declaration ------------------------------------------------
template <typename T> 
concept DevOrInstExtensionType = std::is_same_v<T, vkdefs::DeviceExtension> || std::is_same_v<T, vkdefs::InstanceExtension>;

// From Sascha Willelms example
struct VulkanFrameObjects
{
    VkCommandBuffer commandBuffer;
    VkFence renderCompleteFence;
    VkSemaphore renderCompleteSemaphore;
    VkSemaphore presentCompleteSemaphore;
    bool* windowWasResized;
};

using VulkanCreateFailureCallback = void(*)(std::string_view errorCode, std::string_view info, void* userData);

// TODO create a macro for restrict for different compilers (though right now code works only in clang 16)
auto logAndAbort([[maybe_unused]] std::string_view const errorCode, [[maybe_unused]] std::string_view const description, 
                 [[maybe_unused]] void* userData) -> void
{
    // TODO log error file __FILE__ : line __LINE__ : Fatal: VkResult is errorCode. Aborting now
    std::abort();
}


template <typename SceneDataT, typename Deleter> 
    requires (std::is_nothrow_invocable_r<void, Deleter, typename std::unique_ptr<SceneDataT>::pointer>::value 
        && std::derived_from<SceneDataT, SceneDataTag>)
using ResourcesPreparationFunction = void(*)(std::unique_ptr<SceneDataT, Deleter> pScene, void* pUserData, Status* pOutStatus);

using CommandBufferBuildingFunction = void(*)(void* pUserData, Status* pOutStatus);

// TODO the Extension class needs to be reflected with refl library. Therefore use enable_if to make the requirement 
template <typename T, typename SceneDataT, typename SceneUpdateT, typename TaskType> 
concept VulkanRendererResourceData = 
    std::semiregular<T> && 
    std::derived_from<SceneDataT, SceneDataTag> && 
    std::derived_from<SceneUpdateT, SceneUpdateTag> && 
    requires(T data, VkDevice device, VmaAllocator alloc, SceneUpdateT const& sceneUpdate, TaskType const& task,
             VkExtent2D newWinExtent, Eigen::Matrix4f const& viewMat, void(*deleter)(typename std::unique_ptr<SceneDataT>::pointer))
    {
        {data.createResourcesPreparationFunction(device, alloc)} -> std::same_as<std::span<ResourcesPreparationFunction<decltype(deleter),SceneDataT>>>;
        {data.createCommandBuffersBuildingFunction(device, alloc)} -> std::same_as<std::span<CommandBufferBuildingFunction>>;
        {data.createBaseFrameObjects()} -> std::same_as<Status>;

        {data.onUpdateRenderData(sceneUpdate)} -> std::same_as<Status>;
        {data.onWindowResized(newWinExtent)} -> std::same_as<Status>;
        {data.onViewChanged(viewMat)} -> std::same_as<Status>;
        {data.onViewChanged()} -> std::same_as<Status>;
        {data.onNextFrame()} -> std::same_as<Status>;
        {data.onDestroy()} -> std::same_as<void>;

        {data.bindAndsubmit(task)};
    };

// the allocator given here is also capable to be converted to any other allocator of the same type, but with different template arguments
// thanks to a templated converting constructor
// TODO how can I be assured that only one vulkan instance is created
template <typename Allocator, template <class> typename ExtensionClass>
    requires (allocatorAligned<Allocator>
             && VulkanRendererResourceData<ExtensionClass<Allocator>, SceneDataTag, SceneUpdateTag, TaskTag>)
class VulkanRenderer
{
public:
    struct Options;
    using DebugCallbackType = VKAPI_ATTR VkBool32 VKAPI_CALL (*)(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                              VkDebugUtilsMessageTypeFlagsEXT type,
                                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                              void* pUserData);
    using FeatureCheckCallbackType = bool(*)(VkPhysicalDeviceFeatures2 const& features);
    using QueueFamilyCheckCallback = bool(*)(VkQueueFamilyProperties2 const& queueProperties, VkBool32 supportsPresentation);
    struct ConstructorOptions;
    struct DeviceCreationOptions;
public:
    // constructor takes care of Instance creation and physical device choice
    // requested layers are ignored in release mode.
    // note that device extensions passed here are the ones that do not belong to the pNext chain of VkPhysicalDeviceFeatures2, 
    VulkanRenderer(std::span<vkdefs::InstanceExtension> requestedInstanceExtensions, std::span<std::string_view> requestedLayers, 
                   ConstructorOptions const& options, Allocator const& alloc);

    // TODO copy/move constructor and assignment ops. 
    // This calls onDestroy
    ~VulkanRenderer();

    // some of the extensions specified here may add features. Specify Which features you enable alongside the extensions
    [[nodiscard]] auto createDevice(std::span<vkdefs::DeviceExtension> extensions, DeviceCreationOptions const& options = defaultDeviceCreationOptions) -> Status;

    // calls onNextFrame
    template <typename GPUTaskType, typename GPUTask2, typename Data> 
        requires std::derived_from<GPUTaskType, RenderTask<GPUTask2, Data>>
    [[nodiscard]] auto submit(const GPUTaskType& task) -> VulkanFrameObjects;

    template <typename GPUTaskType, typename GPUTask2, typename Data> 
        requires (!std::derived_from<GPUTaskType, RenderTask<GPUTask2, Data>>)
    [[nodiscard]] auto submit(const GPUTaskType& task) -> Status;

    // this calls ExtensionClass::onViewChanged
    [[nodiscard]] auto onViewChanged(Eigen::Matrix4f const& newViewTransform) -> Status;
        
    // this calls ExtensionClass::onWindowResized and VulkanRenderer::onViewChanged
    [[nodiscard]] auto onWindowResized(VkExtent2D newWindowExtent) -> Status;

    template <std::ranges::contiguous_range Range>
    [[nodiscard]] auto onUpdateRenderData(SceneUpdate<Range> const& sceneDataUpdate) -> Status;

    // this calls the three create functions in the ExtensionClass
    template <typename SceneDataT, typename Deleter>
    struct prepareReturnType
    {
        std::span<ResourcesPreparationFunction<SceneDataT, Deleter>> resPrepFunctions;
        std::span<CommandBufferBuildingFunction> cmdBufBuildFunctions;
    };
    template <typename SceneDataT, typename Deleter> 
        requires (std::is_nothrow_invocable_r<void, Deleter, typename std::unique_ptr<SceneDataT>::pointer>::value &&
                  std::derived_from<SceneDataT, SceneDataTag>)
    [[nodiscard]] auto prepare(std::unique_ptr<SceneDataT, Deleter> pScene, void* pSceneAddOnData, void* pCommandBufferAddOnData, Status* pOutStatus) 
        -> prepareReturnType<SceneDataT, Deleter>;
    // guide for format properties and feature support: first you build up the structure, by specifying the entire pNext chain and sType
    // for each element in the chain, leave the rest undefined. Then after the API call, either vkGetPhysicalDeviceFormatProperties2
    // or vkGetPhysicalDeviceFeatures2, the structs will be fully populated with boolean values in each struct the pNext chain, which you
    // check for support
    // Options has some C-stype pointer and size members instead of templetized random access ranges because I want to avoid a class template
    // within a class template. If I really want a range, I can bring out this options struct from The VulkanRenderer class. I don't know
    // struct with members not ordered per size, but readability, since this is used only at renderer creation I don't care
    struct ConstructorOptions 
    {
        VulkanCreateFailureCallback createFailureCallback;
        void* createFailureCallbackUserData;

        std::conditional_t<debugMode, PFN_vkDebugUtilsMessengerCallbackEXT, nullptr_t> debugUtilsMessengerCallback;
        std::conditional_t<debugMode, void*, nullptr_t> debugUtilsMessengerCallbackUserData;

        VkAllocationCallbacks const* pAllocationCallbacks;

        // the extensions provided must be also be passed as requested extensions to the constructor. Validity is checked in debug mode only
        // VkInstanceCreateInfo::pNext MUST BE NULL in Vulkan 1.3typedef VkBool32 (VKAPI_PTR *PFN_vkDebugUtilsMessengerCallbackEXT)(
        // void const* pNextInstance; // value of the pNext pointer in the InstanceCreateInfo struct.

        // that's the only thing from VkPhysicalDeviceProperties2 which can be requested from the user, all the others are inferred from the 
        // requested extensions and stored for later checks during resource creation, checks only performed in debug mode
        // I am not considering sparse resources

        // desired physical device features
        // REMEMBER: If the pNext chain includes a VkPhysicalDeviceFeatures2 structure, then pEnabledFeatures must be NULL
        VkPhysicalDeviceFeatures2& requestedFeatures;

        // give a function or stateless lambda which check that the features you require are present
        FeatureCheckCallbackType featureCheckCallback;

        void const* pNextDevice; // value of the pNext pointer in the deviceCreateInfo struct.
        
        // Surface on which to check against support of window system integration presentation (VK_KHR_surface, VK_KHR_swapchain required)
        // Doesn't take ownership of the surface. If VK_NULL_HANDLE, then assumed user is not using aforementioned extensions
        VkSurfaceKHR surface; // TODO THIS NEEDS TO BE REFACTORED OUT ALONG WITH THE CODE IN THE CONSTRUCTOR SUCH 

        // requested family properties. User defines up to 4 checks on the queue families found on each physical device, and defines which 
        // properties to query for each family by supplying a VkQueueFamilyProperties2 struct which will be used as a "blueprint" to allocate
        // QueueFamilyPropertyCount properties struct. This means you activate the same extensions for all queue families but perform different, 
        // up to four, check functions on them
        vkdefs::StaticVector<QueueFamilyCheckCallback, 4> const& requestedFamilyPropertiesCallbacks;
        VkQueueFamilyProperties2 const& queuePropertiesBlueprint;
    };

    static inline ConstructorOptions defaultConstructorOptions { 
        logAndAbort, nullptr, nullptr, nullptr, nullptr, vkdefs::noFeatures, 
        [](VkPhysicalDeviceFeatures2 const&){return true;}, nullptr, VK_NULL_HANDLE, vkdefs::graphicsTransfer, 1
    };

    // necessary second step, where the user specifies desired queue family properties, along with queueCount, and desired queue family
    // properties
    struct DeviceCreationOptions
    {
        // these features must be the same passed in the constructor, or a subset
        VkPhysicalDeviceFeatures2 const& requestedFeatures;

        // in Vulkan 1.3, this is the only possible pNext for VkDeviceQueueCreateInfo
        vkdefs::StaticVector<VkDeviceQueueGlobalPriorityCreateInfoKHR const*, 4> deviceQueueCI_pNexts; 

        // TODO don't care for queue priorities for now or protected bits
    };
    static inline DeviceCreationOptions defaultDeviceCreationOptions = { vkdefs::noFeatures, {nullptr, nullptr, nullptr, nullptr} };

private: // utility functions

    template <typename Callable> requires std::is_nothrow_invocable_r<void, Callable>::value
    constexpr auto checkExtensionDependenciesOtherwise(std::span<vkdefs::InstanceExtension> requestedInstanceExtensions, 
                                                       std::span<vkdefs::DeviceExtension> requestedDeviceExtensions, Callable&& otherwiseCallable
    ) const -> bool;

    auto checkVkResult(VkResult const res, std::string_view const msg) -> void;

    // TODO allocator to std vector
    auto copyPhysicalDeviceFeatures(VkPhysicalDeviceFeatures2 const& features,
                                    std::vector<vkdefs::DeviceFeatureChainNode>& outFeatures) -> void;

    auto copyQueueFamilyProperties_dbg(vkdefs::StaticVector<int32_t, 4> const& selectedQueueFamilies, VkQueueFamilyProperties2 const* MXC_restrict pQueueFamilyProperties) -> void;

    // delegating constructor with the default member initializer list
    VulkanRenderer();

private:
    // always used 
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    struct Queue // is it necessary to store all this information?
    {
        VkQueue handle;
        VkQueueFlags family;
        uint32_t familyIndex;
        int32_t index;
        VkBool32 presentationSupported;
    };
    vkdefs::StaticVector<Queue, 4> m_queues; // how many queues should I create
    VmaAllocator m_vmaAllocator;

    // stored to check for compatibility with a swapchain
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    VkAllocationCallbacks* m_pAllocationCallbacks = nullptr;
    VkAllocationCallbacks m_allocationCallbacks{};
    VulkanCreateFailureCallback m_createFailureCallback = nullptr;
    void* m_createFailureCallbackUserData = nullptr;

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap46.html#limits-minmax
    // refer to table 55 of the link to see required limits for which you are not required to check against
    // TODO allocator aware std vector with union?? Refactor this madness
    std::vector<vkdefs::DeviceFeatureChainNode> m_supportedFeatures;
    std::vector<vkdefs::DeviceFeatureChainNode> m_enabledFeatures;

#ifndef NDEBUG
    VkDebugUtilsMessengerEXT m_messenger_dbg; // used only if 1) VK_EXT_debug_utils is supported and requested and we are in debug mode
    bool m_messengerConstructed_dbg;
    DebugCallbackType m_debugCallback_dbg;
#endif

    // for each queue family we store its properties, only in debug, BUT such properties are checked against if some extensions are enabled
    // example, the VK_KHR_sparse_binding extensions is enabled, we should check whether there is a queue with flag VK_QUEUE_SPARSE_BINDING_BIT
    struct QueueFamilyPropertiesChain 
    { 
        std::vector<vkdefs::QueueFamilyPropertiesChainNode> vec;
        uint32_t index; // Family index != queueIndex
    };
    std::vector<QueueFamilyPropertiesChain> m_queueFamilyProperties_dbg; // checked to see if a queue has specific flags

    // necessary data passed at construction which is needed to create devices
    std::vector<vkdefs::InstanceExtension> m_instanceExtensions;
    std::vector<vkdefs::DeviceExtension> m_deviceExtensions;

    ExtensionClass<Allocator> m_data;

    Allocator m_alloc;

    // given to swapchain instance, which will set is after a present operation. Not atomic because presentation happens on a single thread
    bool windowWasResized = false; 
};

// -------------------- Vulkan Renderer Utilities -------------------------------------------------
//
// if you ever change the std::abort strategy, change the allocation method to not leak memory
template <typename Allocator, template <class> typename ExtensionClass>
    requires (allocatorAligned<Allocator>
             && VulkanRendererResourceData<ExtensionClass<Allocator>, SceneDataTag, SceneUpdateTag, TaskTag>)
template <typename Callable> requires std::is_nothrow_invocable_r<void, Callable>::value
constexpr auto VulkanRenderer<Allocator, ExtensionClass>::checkExtensionDependenciesOtherwise(
    std::span<vkdefs::InstanceExtension> requestedInstanceExtensions, 
    std::span<vkdefs::DeviceExtension> requestedDeviceExtensions, 
    Callable&& otherwiseCallable) const -> bool
{
    // --------- declare lambdas to check extension dependencies ---------------
    bool ret = true;
    constexpr auto checkDependenciesFor = [&]<DevOrInstExtensionType ExtType>(ExtType extension)->void {
        std::optional<vkdefs::DependencyVec> dependencies = vkdefs::extensionDependencyMap_dbg.at(extension); // constructing a variant
        if (dependencies)
        {
            for (uint32_t i = 0; i != dependencies->size(); ++i)
            {
                vkdefs::Extension const& requirement = (*dependencies)[i];
                std::visit([&](DevOrInstExtensionType auto&& ext)->void{
                    using Type = std::decay_t<decltype(ext)>; // remove any cv qualifier and reference
                    if constexpr (std::is_same_v<Type, vkdefs::DeviceExtension>)
                    {
                        auto const iter = vkutils::findOr(std::forward<std::decay_t<decltype(requestedDeviceExtensions)>>(requestedDeviceExtensions), 
                                                          ext, otherwiseCallable);
                        ret = iter == requestedDeviceExtensions.end();
                    }
                    else
                    {
                        auto const iter = vkutils::findOr(std::forward<std::decay_t<decltype(requestedInstanceExtensions)>>(requestedInstanceExtensions), 
                                                          ext, otherwiseCallable);
                        ret = iter == requestedInstanceExtensions.end();
                    }
                }, requirement);
            }
        }
    };

    // ------------ check the dependencies for each extension ------------------
    for (auto const& extension : requestedInstanceExtensions)
    {
        // checkDependenciesFor.template operator()<std::decay_t<decltype(extension)>>(extension); use this if cannot deduce types
        checkDependenciesFor(extension);
    }
    for (auto const& extension : requestedDeviceExtensions)
    {
        checkDependenciesFor(extension);
    }

    return ret;
}

template <typename Allocator, template <class> typename ExtensionClass>
    requires (allocatorAligned<Allocator>
             && VulkanRendererResourceData<ExtensionClass<Allocator>, SceneDataTag, SceneUpdateTag, TaskTag>)
auto VulkanRenderer<Allocator, ExtensionClass>::copyPhysicalDeviceFeatures(
    VkPhysicalDeviceFeatures2 const& features, 
    std::vector<vkdefs::DeviceFeatureChainNode>& outFeatures) -> void
{
    assert(outFeatures.size() == 0);

    // copy the first struct
    outFeatures.emplace_back(features);
    if (features.pNext == nullptr)
    {
        return;
    }

    // then copy each element in the pNext Chain until you hit a nullptr
    auto pCurrentFeatureStruct = reinterpret_cast<vkdefs::SlicedVkStruct const*>(features.pNext);
    while (true)
    {
        outFeatures.emplace_back(pCurrentFeatureStruct);
        if (pCurrentFeatureStruct->pNext == nullptr)
        {
            break;
        }
        pCurrentFeatureStruct = reinterpret_cast<vkdefs::SlicedVkStruct const*>(pCurrentFeatureStruct->pNext);
    }
}

template <typename Allocator, template <class> typename ExtensionClass>
    requires (allocatorAligned<Allocator>
             && VulkanRendererResourceData<ExtensionClass<Allocator>, SceneDataTag, SceneUpdateTag, TaskTag>)
auto VulkanRenderer<Allocator, ExtensionClass>::copyQueueFamilyProperties_dbg(vkdefs::StaticVector<int32_t, 4> const& selectedQueueFamilies, 
                                                              VkQueueFamilyProperties2 const* MXC_restrict pQueueFamilyProperties) -> void
{
#ifndef NDEBUG
    assert(m_queueFamilyProperties_dbg.size() == 0);
    for (uint32_t queueIndex = 0; queueIndex != selectedQueueFamilies.size(); ++queueIndex)
    {
        if (queueIndex == -1)
            continue;

        QueueFamilyPropertiesChain chainVector;
        chainVector.index = queueIndex;
        chainVector.vec.reserve(4);

        VkQueueFamilyProperties2 const* current = (pQueueFamilyProperties + queueIndex);
        assert(current);
        chainVector.vec.emplace_back(*current);
        auto currentSliced = reinterpret_cast<vkdefs::SlicedVkStruct const*>(current);
        
        while (currentSliced->pNext)
        {
            currentSliced = currentSliced->pNext;
            chainVector.vec.emplace_back(currentSliced);
        }

        m_queueFamilyProperties_dbg.push_back(std::move(chainVector));
    }
#endif
}

template <typename Allocator, template <class> typename ExtensionClass>
auto VulkanRenderer<Allocator, ExtensionClass>::checkVkResult(VkResult const res, std::string_view const msg) -> void
{
    if (res != VK_SUCCESS) 
        m_createFailureCallback(vkutils::errorString(res), msg, m_createFailureCallbackUserData);
}

// ------------------- Vulkan Renderer Implementation ---------------------------------------------

template <typename Allocator, template <class> typename ExtensionClass>
    requires (allocatorAligned<Allocator>
             && VulkanRendererResourceData<ExtensionClass<Allocator>, SceneDataTag, SceneUpdateTag, TaskTag>)
VulkanRenderer<Allocator, ExtensionClass>::~VulkanRenderer()
{
    m_data.destroy();

    vkDestroyDevice(m_device, m_pAllocationCallbacks);

#ifndef NDEBUG
    if (m_messengerConstructed_dbg)
    {
        auto const pfnDestroyDebugUtilsMessengerEXT = 
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
        pfnDestroyDebugUtilsMessengerEXT(m_instance, m_messenger_dbg, &m_allocationCallbacks);
    }
#endif

    vkDestroyInstance(m_instance, &m_allocationCallbacks);
}

// TODO reduce stack space usage by creating arbitrary scopes and multiple cleanup sections
// TODO reduce memory waste by pre allocating a big block and then distributing it across arrays
// TODO add checks on each allocation
template <typename Allocator, template <class> typename ExtensionClass>
    requires (allocatorAligned<Allocator>
             && VulkanRendererResourceData<ExtensionClass<Allocator>, SceneDataTag, SceneUpdateTag, TaskTag>)
VulkanRenderer<Allocator, ExtensionClass>::VulkanRenderer(
    std::span<vkdefs::InstanceExtension> requestedInstanceExtensions, 
    std::span<std::string_view> requestedLayers, 
    ConstructorOptions const& options, 
    Allocator const& alloc) 
    : VulkanRenderer()
{
    m_alloc = alloc;
    m_surface = options.surface;
    m_createFailureCallback = options.createFailureCallback;
    m_createFailureCallbackUserData = options.createFailureCallbackUserData;
    if (options.pAllocationCallbacks)
    {
        std::memcpy(&m_allocationCallbacks, &options.pAllocationCallbacks, sizeof(VkAllocationCallbacks));
        m_pAllocationCallbacks = &m_allocationCallbacks;
    }

    // ----------------------- Instance Creation ------------------------------
    // API Version format: 31-29 variant | 28-22 major | 21-12 minor | 11-0 patch
    uint32_t apiVersion;
    vkEnumerateInstanceVersion(&apiVersion);
    // strip away the variant and check if the version is bigger than 1.3
    if ((apiVersion & 0x1fff'ffff) < VK_API_VERSION_1_3) [[unlikely]]
    {
        // TODO: write to a "custom" debug console?
        std::abort(); // TODO: is this appropriate?
    }

    VkApplicationInfo const appInfo = vkdefs::applicationInfo();
    VkInstanceCreateInfo instanceCI = vkdefs::instanceCreateInfo();
    instanceCI.pApplicationInfo = &appInfo;
    
    // process Instance Extensions
    if constexpr (debugMode)
    {
        auto logAndAbort = [](){
            // TODO insert log here. (debug thing with fmt)
            std::abort();
        };
        checkExtensionDependenciesOtherwise(requestedInstanceExtensions, {}, logAndAbort);
    }

    uint32_t extensionProperties_count;
    vkEnumerateInstanceExtensionProperties(nullptr/*layer*/, &extensionProperties_count, nullptr);
    assert(extensionProperties_count != 0u);

    auto pExtensionProperties = reinterpret_cast<VkExtensionProperties const*>(m_alloc.allocateAligned(extensionProperties_count * sizeof(VkExtensionProperties), alignof(VkExtensionProperties)));
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionProperties_count, pExtensionProperties);

    // check for extension compatibility
    auto supportedInstanceExtensions = reinterpret_cast<char const**>(m_alloc.allocateAligned(requestedInstanceExtensions.size() * sizeof(char const*), alignof(char const*)));
    uint32_t supportedInstanceExtensions_count = 0;
    for (uint32_t i = 0; i != requestedInstanceExtensions.size(); ++i)
    {
        *(supportedInstanceExtensions+i) = vkdefs::instanceExtensionToString(requestedInstanceExtensions[i]);
    }

    // not using std::find because <algorithm> is included only in debug mode (see checkExtensionDependenciesOtherwise)
#ifndef NDEBUG
    bool containsDebugUtils;
#endif
    for (uint32_t i = 0; i != requestedInstanceExtensions.size(); ++i)
    {
        std::string_view const inputExtension(vkdefs::instanceExtensionToString(requestedInstanceExtensions[i]));
        std::string_view supportedExtension;
        bool supported = false;
        for (uint32_t j = 0; j != extensionProperties_count; ++j)
        {
            static_assert(std::is_same_v<decltype(pExtensionProperties[j].extensionName), char[VK_MAX_EXTENSION_NAME_SIZE]>);
            supportedExtension = pExtensionProperties[j].extensionName; // takes up to the nul character, exclusive
            if (inputExtension == supportedExtension)
            {
            #ifndef NDEBUG
                if (inputExtension == "VK_EXT_debug_utils")
                    containsDebugUtils = true;
            #endif
                supported = true;
                std::strncpy(*(supportedInstanceExtensions+i), supportedExtension.data(), VK_MAX_EXTENSION_NAME_SIZE);
                ++supportedInstanceExtensions_count;
                break;
            }
        }
        
        if (!supported)
        {
            // TODO insert log here
        }
    }
    instanceCI.ppEnabledExtensionNames = supportedInstanceExtensions;
    instanceCI.enabledExtensionCount = supportedInstanceExtensions_count;

    // check for validation layers, debug only
    std::conditional_t<debugMode, char const**, nullptr_t> pEnabledLayers = nullptr;
    std::conditional_t<debugMode, VkLayerProperties*, nullptr_t> pSupportedLayers = nullptr;
    uint32_t enabledLayers_count = 0;
    uint32_t supportedLayers_count = 0;
    #ifndef NDEBUG
        pEnabledLayers = reinterpret_cast<char const**>(m_alloc.allocateAligned(requestedLayers.size() * sizeof(char const*), alignof(char const*)));
        vkEnumerateInstanceLayerProperties(&supportedLayers_count, nullptr);
        if (supportedLayers_count != 0) [[likely]] 
        {
            pSupportedLayers = reinterpret_cast<VkLayerProperties*>(m_alloc.allocateAligned(supportedLayers_count * sizeof(VkLayerProperties), alignof(VkLayerProperties)));
            vkEnumerateInstanceLayerProperties(&supportedLayers_count, pSupportedLayers);

            for (uint32_t i = 0; i != requestedLayers.size(); ++i)
            {
                std::string_view const& inputLayer(requestedLayers[i]);
                std::string_view supportedLayer;
                bool supported = false;
                for (uint32_t j = 0; j != supportedLayers_count; ++i)
                {
                    supportedLayer = pSupportedLayers[j].layerName;
                    if (inputLayer == supportedLayer)
                    {
                        std::strncpy(pEnabledLayers[i], inputLayer.data(), VK_MAX_EXTENSION_NAME_SIZE);
                        ++enabledLayers_count;
                        supported = true;
                        break;
                    }
                }

                if (!supported)
                {
                    // TODO insert log here
                }
            }

            instanceCI.enabledLayerCount = enabledLayers_count;
            instanceCI.ppEnabledLayerNames = pEnabledLayers;
        }
        // TODO insert log here
        instanceCI.enabledLayerCount = 0;
        instanceCI.ppEnabledLayerNames = nullptr;
    #else
        instanceCI.enabledLayerCount = 0;
        instanceCI.ppEnabledLayerNames = nullptr;
    #endif

    VkResult res = vkCreateInstance(&instanceCI, options.pAllocationCallbacks, &m_instance);
    checkVkResult(res, "failed to create VkInstance");
    
    // --------------- Setup VK_EXT_debug_utils messenger (debug only) --------
#ifndef NDEBUG
    if (containsDebugUtils)
    {
        auto const pfnCreateDebugUtilsMessengerEXT = 
            reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
        auto messengerCI = vkdefs::debugUtilsMessengerCreateInfo_dbg(); // if you want to check instance creation too, put this in pNext.

        res = pfnCreateDebugUtilsMessengerEXT(m_instance, &messengerCI, m_pAllocationCallbacks, &m_messenger_dbg);
        checkVkResult(res, "failed to create VkDebugUtilsMessengerEXT");
        m_messengerConstructed_dbg = true;
    }
#endif

    // -------------- Choose Physical Device ----------------------------------
    // get physical device list
    uint32_t availPhysicalDevices_count = 0;
    res = vkEnumeratePhysicalDevices(m_instance, &availPhysicalDevices_count, nullptr);
    checkVkResult(res, "couldn't enumerate physical devices");
    if (availPhysicalDevices_count == 0) [[unlikely]]
    {
        // TODO insert log here
        std::abort();
    }

    auto pAvailPhysicalDevices = reinterpret_cast<VkPhysicalDevice*>(m_alloc.allocateAligned(availPhysicalDevices_count * sizeof(VkPhysicalDevice), alignof(VkPhysicalDevice)));
    res = vkEnumeratePhysicalDevices(m_instance, &availPhysicalDevices_count, pAvailPhysicalDevices);
    checkVkResult(res, "couldn't save physical devices list in local buffer");

    // select the first physical device which supports all the requested features and extensions, and that has requested queue families 
    for (uint32_t i = 0; i != availPhysicalDevices_count; ++i)
    {
        VkPhysicalDevice currentPhysicalDevice = pAvailPhysicalDevices[i];

        // TODO store properties, memoryproperties, features, queuefamilyproperties in VulkanRenderer in debug mode
        // physical device properties can be useful to check if every operation or resource created is within the capabilities of the device, e.g.
        // check that you are not creating more VkMemory objects than maxMemoryAllocationCount. Not doing it now as requires a lot of time
        // vkGetPhysicalDeviceProperties2(currentPhysicalDevice, );

        // Pay attenction to chapter 32.1 of the Vulkan specification, which lists all Features which are mandatory for a valid vulkan implementation
        // therefore we don't need to check for them
        vkGetPhysicalDeviceFeatures2(currentPhysicalDevice, &options.requestedFeatures);
        if (!options.featureCheckCallback(options.requestedFeatures))
        {
            continue;
        }

        // save supported features in m_supportedFeatures to check later for device creation
        copyPhysicalDeviceFeatures(options.requestedFeatures, m_supportedFeatures);

        // check for the requested family properties
        uint32_t queueFamilyProperties_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties2(currentPhysicalDevice, &queueFamilyProperties_count, nullptr);
        if (queueFamilyProperties_count == 0)
        {
            continue;
        }

        vkdefs::StaticVector<int32_t, 4> selectedQueueFamilies; // TODO StaticVector doesn't have a constructor which takes value and size yet
        for (uint32_t i = 0; i != options.requestedFamilyPropertiesCallbacks.size(); ++i)
        {
            selectedQueueFamilies.push_back(-1);
        }

        auto pQueueFamilyProperties = reinterpret_cast<VkQueueFamilyProperties2*>(m_alloc.allocateAligned(queueFamilyProperties_count * sizeof(VkQueueFamilyProperties2), alignof(VkQueueFamilyProperties2*)));
        {
            for (uint32_t i = 0; i != queueFamilyProperties_count; ++i)
            {
                std::memcpy(pQueueFamilyProperties+i, &options.queuePropertiesBlueprint, sizeof(VkQueueFamilyProperties2));
            }
            vkGetPhysicalDeviceQueueFamilyProperties2(currentPhysicalDevice, &queueFamilyProperties_count, pQueueFamilyProperties);

            for (uint32_t queueFamilyIndex = 0, lastUpdated = 0; 
                 queueFamilyIndex != queueFamilyProperties_count && lastUpdated != selectedQueueFamilies.size(); 
                 ++queueFamilyIndex) // for each queue family properties, until all necessary queue families have been selected
            {
                VkBool32 presentationSupported = VK_FALSE;
                if (options.surface != VK_NULL_HANDLE)
                {
                    PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR = vkGetInstanceProcAddr(m_instance, u8"vkGetPhysicalDeviceSurfaceSupportKHR");
                    fpGetPhysicalDeviceSurfaceSupportKHR(currentPhysicalDevice, queueFamilyIndex, options.surface, &presentationSupported);
                    // won't perform surface queries here, because they are done in the swapchain class
                }

                for (uint32_t j = lastUpdated; j != options.requestedFamilyPropertiesCallbacks.size(); ++j) // for each condition requirement
                {
                    // TODO: now this framework suppports 1 queue per queue family. Maybe support for more??
                    QueueFamilyCheckCallback const& callback = options.requestedFamilyPropertiesCallbacks[queueFamilyIndex];
                    if (callback(*pQueueFamilyProperties, presentationSupported))
                    {
                        selectedQueueFamilies[lastUpdated++] = queueFamilyIndex;
                        m_queues.push_back( // TODO StaticVector doesn't have emplace back yet
                            Queue {
                                .handle = VK_NULL_HANDLE, 
                                .family = pQueueFamilyProperties->queueFamilyProperties.queueFlags,
                                .familyIndex = queueFamilyIndex,
                                .index = 0,
                                .presentationSupported = presentationSupported
                        });

                        break;
                    }
                }
            }

            // in debug mode, store the whole VkQueueFamilyProperties2 chain so that we can use it, along with the enabled Instance Extensions,
            // if subsequent device creations are fine
            if constexpr (debugMode)
            {
                copyQueueFamilyProperties_dbg(selectedQueueFamilies, pQueueFamilyProperties);
            }
        }
        m_alloc.deallocate(pQueueFamilyProperties, queueFamilyProperties_count * sizeof(VkQueueFamilyProperties2));

        if (selectedQueueFamilies[0] == -1)
        {
            continue;
        }

        // Assignment step
        m_physicalDevice = currentPhysicalDevice;

        break;
    }

    if (m_physicalDevice == VK_NULL_HANDLE)
    {
        // TODO log 
        std::abort();
    }

    // ------------------- store requested instance extensions ----------------
    assert(m_instanceExtensions.size() == 0);
    m_instanceExtensions.reserve(requestedInstanceExtensions.size());
    for (vkdefs::InstanceExtension const& insExt : requestedInstanceExtensions)
    {
        m_instanceExtensions.push_back(insExt);
    }

    // ---------------------- Cleanup -----------------------------------------
    m_alloc.deallocate(supportedInstanceExtensions, requestedInstanceExtensions.size() * sizeof(char const*)); // from process Instance Extensions
    m_alloc.deallocate(pExtensionProperties, extensionProperties_count * sizeof(VkExtensionProperties)); // from process Instance Extensions
    if constexpr (debugMode)
    {
        m_alloc.deallocate(pEnabledLayers, requestedLayers.size() * sizeof(char const*));
        m_alloc.deallocate(pSupportedLayers, supportedLayers_count * sizeof(VkLayerProperties));
    }
    m_alloc.deallocate(pAvailPhysicalDevices, availPhysicalDevices_count * sizeof(VkPhysicalDevice));
}
 
// TODO maybe some checks
// VkPhysicalDeviceVulkanMemoryModelFeatures
template <typename Allocator, template <class> typename ExtensionClass>
    requires (allocatorAligned<Allocator>
             && VulkanRendererResourceData<ExtensionClass<Allocator>, SceneDataTag, SceneUpdateTag, TaskTag>)
[[nodiscard]] auto VulkanRenderer<Allocator, ExtensionClass>::createDevice(
    std::span<vkdefs::DeviceExtension> extensions, 
    DeviceCreationOptions const& options) 
    -> Status
{
    // -------------- Setup Device Create Info --------------------------------
    VkDeviceCreateInfo deviceCreateInfo = vkdefs::deviceCreateInfo();
    deviceCreateInfo.pNext = reinterpret_cast<void const*>(&options.requestedFeatures);
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());

    std::vector<char const*> pExtensionNames;
    pExtensionNames.reserve(extensions.size());

    for (auto const& ext : extensions)
        pExtensionNames.push_back(vkdefs::deviceExtensionToString(ext).data());

    deviceCreateInfo.ppEnabledExtensionNames = pExtensionNames.data();
    
    deviceCreateInfo.queueCreateInfoCount = m_queues.size();
    std::array<VkDeviceQueueCreateInfo, 4> queueCreateInfos{vkdefs::deviceQueueCreateInfo()};
    float const queuePriority = 1.0f;

    for (uint32_t i = 0; i != m_queues.size(); ++i)
    {
        queueCreateInfos[i].pNext = options.deviceQueueCI_pNexts[i];
        queueCreateInfos[i].queueFamilyIndex = m_queues[i].familyIndex;
        queueCreateInfos[i].queueCount = 1;
        queueCreateInfos[i].pQueuePriorities = &queuePriority;
    }

    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

    // ----------- Create Device ----------------------------------------------
    VkResult res = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, m_pAllocationCallbacks, &m_device);
    checkVkResult(res, "failed to create VkResult");

    copyPhysicalDeviceFeature(options.requestedFeatures, m_enabledFeatures);

    // ---------- Get Device Queues -------------------------------------------
    vkdefs::StaticVector<VkDeviceQueueInfo2, 4> queueInfos; // TODO StaticVector doesn't support emplace_back yet
    for (uint32_t i = 0; i != m_queues.size(); ++i)
    {
        queueInfos[i].queueFamilyIndex = m_queues[i].familyIndex;
        queueInfos[i].queueIndex = 0;

        vkGetDeviceQueue2(m_device, &queueInfos[i], &m_queues[i].handle);
    }

    // ---------- Copy Device Extensions ---------------------------------------
    m_deviceExtensions.resize(extensions.size());
    std::memcpy(m_deviceExtensions.data(), extensions.data(), extensions.size() * sizeof(vkdefs::DeviceExtension));

    return Status::SUCCESS;
}

// calls onNextFrame
template <typename Allocator, template <class> typename ExtensionClass>
    requires (allocatorAligned<Allocator>
             && VulkanRendererResourceData<ExtensionClass<Allocator>, SceneDataTag, SceneUpdateTag, TaskTag>)
template <typename GPUTaskType, typename GPUTask2, typename Data> 
    requires std::derived_from<GPUTaskType, RenderTask<GPUTask2, Data>>
[[nodiscard]] auto VulkanRenderer<Allocator, ExtensionClass>::submit(const GPUTaskType& task) -> VulkanFrameObjects
{
    if (Status ret = m_data.onNextFrame(); ret != Status::SUCCESS) [[unlikely]]
    {
        return VulkanFrameObjects{};
    }
    return m_data.bindAndSubmit(task);
}

template <typename Allocator, template <class> typename ExtensionClass>
    requires (allocatorAligned<Allocator>
             && VulkanRendererResourceData<ExtensionClass<Allocator>, SceneDataTag, SceneUpdateTag, TaskTag>)
template <typename GPUTaskType, typename GPUTask2, typename Data> 
    requires (!std::derived_from<GPUTaskType, RenderTask<GPUTask2, Data>>)
[[nodiscard]] auto VulkanRenderer<Allocator, ExtensionClass>::submit(const GPUTaskType& task) -> Status
{
    if (Status ret = m_data.onNextFrame(); ret != Status::SUCCESS) [[unlikely]]
    {
        return ret;
    }
    return m_data.bindAndSubmit(task);
}

// this calls ExtensionClass::onViewChanged
template <typename Allocator, template <class> typename ExtensionClass>
    requires (allocatorAligned<Allocator>
             && VulkanRendererResourceData<ExtensionClass<Allocator>, SceneDataTag, SceneUpdateTag, TaskTag>)
[[nodiscard]] auto VulkanRenderer<Allocator, ExtensionClass>::onViewChanged(Eigen::Matrix4f const& newViewTransform) -> Status
{
    return m_data.onViewChanged(newViewTransform);
}

// this calls ExtensionClass::onWindowResized and VulkanRenderer::onViewChanged
template <typename Allocator, template <class> typename ExtensionClass>
    requires (allocatorAligned<Allocator>
             && VulkanRendererResourceData<ExtensionClass<Allocator>, SceneDataTag, SceneUpdateTag, TaskTag>)
[[nodiscard]] auto VulkanRenderer<Allocator, ExtensionClass>::onWindowResized(VkExtent2D newWindowExtent) -> Status
{
    if (Status ret = m_data.onWindowResized(newWindowExtent); ret != Status::SUCCESS) [[unlikely]]
    {
        return ret;
    }
    return m_data.onViewChanged();
}

template <typename Allocator, template <class> typename ExtensionClass>
    requires (allocatorAligned<Allocator>
             && VulkanRendererResourceData<ExtensionClass<Allocator>, SceneDataTag, SceneUpdateTag, TaskTag>)
template <std::ranges::contiguous_range Range>
[[nodiscard]] auto VulkanRenderer<Allocator, ExtensionClass>::onUpdateRenderData(SceneUpdate<Range> const& sceneDataUpdate) -> Status
{
    return m_data.onUpdateRenderData(sceneDataUpdate);
}

// this calls the three create functions in the ExtensionClass
template <typename Allocator, template <class> typename ExtensionClass>
    requires (allocatorAligned<Allocator>
             && VulkanRendererResourceData<ExtensionClass<Allocator>, SceneDataTag, SceneUpdateTag, TaskTag>)
template <typename SceneDataT, typename Deleter>
    requires (std::is_nothrow_invocable_r<void, Deleter, typename std::unique_ptr<SceneDataT>::pointer>::value &&
              std::derived_from<SceneDataT, SceneDataTag>)
[[nodiscard]] auto VulkanRenderer<Allocator, ExtensionClass>::prepare(std::unique_ptr<SceneDataT, Deleter> pScene, 
                                                                      void* pSceneAddOnData, void* pCommandBufferAddOnData, Status* pOutStatus) 
    -> prepareReturnType<SceneDataT, Deleter>
{
    // ------------ Create VMA Allocator --------------------------------------
    VmaAllocatorCreateInfo allocatorCreateInfo = vkdefs::vmaAllocatorCreateInfo();
    allocatorCreateInfo.instance = m_instance;
    allocatorCreateInfo.device = m_device;
    allocatorCreateInfo.physicalDevice = m_physicalDevice;
    // TODO finish it

    VkResult res = vmaCreateAllocator(&allocatorCreateInfo, &m_vmaAllocator);
    checkVkResult(res, "failed to create VmaAllocator");

    // ----------- Call the callbacks of the templetized class instance -------
    if (Status ret = m_data.createBaseFramebufferObjects(); ret != Status::SUCCESS) [[unlikely]]
    {
        return ret;
    }
    return {m_data.createResourcesPreparationFunction(m_device, m_vmaAllocator),
            m_data.createCommandBuffersBuildingFunction(m_device, m_vmaAllocator)};
}

} // namespace mxc

#endif // MXC_VULKAN_RENDERER_HPP
