#include "VulkanApplication.h"
#include "Shader.h"
#include "Pipeline.h"
#include "Renderer.h"
#include "Buffer.h"
#include "logging.h"

#include <vector>

static auto constexpr spectrumTestLayer_name = "spectrumTestLayer";

struct SpectrumTestLayer_data
{
	mxc::ShaderSet shaderSet;
	mxc::Pipeline pipeline;
	// TODO make as many as swapchain Images
	// TODO remove vector
	struct ImageInfo { VkDescriptorImageInfo descriptorInfo; VkImageLayout currentLayout;};
	std::vector<ImageInfo> swapchainImageInfos;
	mxc::CommandBuffer layoutTransitionCmdBuf;
};

SpectrumTestLayer_data data;

auto spectrumTestLayer_init(mxc::ApplicationPtr appPtr, void* layerData) -> bool;
auto spectrumTestLayer_tick(mxc::ApplicationPtr appPtr, float deltaTime, void* layerData) -> mxc::ApplicationSignal_t;
auto spectrumTestLayer_shutdown(mxc::ApplicationPtr appPtr, void* layerData) -> void;
auto spectrumTestLayer_handler(mxc::ApplicationPtr appPtr, mxc::EventName name, void* layerData, mxc::EventData eventData) -> mxc::ApplicationSignal_t;

static mxc::ApplicationLayer s_spectrumTestLayer {
	.init = spectrumTestLayer_init,
	.tick = spectrumTestLayer_tick,
	.shutdown = spectrumTestLayer_shutdown,
	.handler = spectrumTestLayer_handler,

	.data = reinterpret_cast<void*>(&data)
};

auto initializeApplication(mxc::VulkanApplication& app, int32_t argc, char** argv) -> bool
{
	app.pushLayer(s_spectrumTestLayer, spectrumTestLayer_name);
	return true;
}

// impl -----------------------------------------------------------------------

auto spectrumTestLayer_init(mxc::ApplicationPtr appPtr, void* layerData) -> bool
{
	MXC_DEBUG("Initialization of Spectrum Test Layer...");
	mxc::VulkanApplication& app = appPtr.get<mxc::VulkanApplication>();
	auto spectrumTestLayerData = reinterpret_cast<SpectrumTestLayer_data*>(layerData);
	auto& renderer = app.getRenderer();
	auto* ctx = renderer.getContextPointer();
	auto& vulkanDevice = ctx->device;

	// create shaders and pipeline --------------------------------------------
	uint32_t swapchainImageCount = ctx->swapchain.images.size();
	static uint32_t constexpr POOLSIZES_COUNT = 1;
	VkDescriptorPoolSize const poolSizes[POOLSIZES_COUNT] {
		{.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 1}
	};
	uint32_t const bindingNumbers[POOLSIZES_COUNT] { 0 };
	mxc::ResourceConfiguration resConfig{};
	resConfig.poolSizes_count = POOLSIZES_COUNT;
	resConfig.pPoolSizes = poolSizes;
	resConfig.pBindingNumbers = bindingNumbers;

	wchar_t const* filenames[] = { SHADER_DIR L"/spectrumTest.comp" };
	VkShaderStageFlagBits stageFlags[] = {VK_SHADER_STAGE_COMPUTE_BIT};
	mxc::ShaderConfiguration shaderConfig{};
	shaderConfig.stage_count = 1;
	shaderConfig.filenames = filenames;
	shaderConfig.stageFlags = stageFlags;

	if (!spectrumTestLayerData->shaderSet.create(ctx, shaderConfig, resConfig))
		return false;

	// prepare update the shaderset with image data ---------------------------
	// TODO do it with push descriptors TODO don't use updateAll
	spectrumTestLayerData->swapchainImageInfos.resize(swapchainImageCount); 
	uint32_t i = 0;
	for (auto& [ descriptorInfo, layout ] : spectrumTestLayerData->swapchainImageInfos)
	{
		descriptorInfo.sampler = VK_NULL_HANDLE;
		descriptorInfo.imageView = ctx->swapchain.images[i++].view;
		descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL; // update done after compute shader transition, before compute shader dispatch
		layout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	auto [width, height] = app.getWindowExtent();
	bool res = spectrumTestLayerData->pipeline.create(
		ctx,
		spectrumTestLayerData->shaderSet,
		width, height
	);

	if (!res)
		return false;

	// create descriptor sets update template ---------------------------------
	uint32_t strides[POOLSIZES_COUNT] { sizeof(uint32_t) }; // VK_FORMAT_B8G8R8A8_UNORM
	spectrumTestLayerData->shaderSet.resources.createUpdateTemplate(ctx, VK_PIPELINE_BIND_POINT_COMPUTE, 
		spectrumTestLayerData->pipeline.layout,
		strides
	);

	// create buffers ---------------------------------------------------------
    MXC_ASSERT(spectrumTestLayerData->layoutTransitionCmdBuf.allocate(ctx, mxc::CommandType::GRAPHICS), 
			   "Couldn't allocate image memory barrier command buffer");

	app.registerHandler(mxc::windowResizedEvent_name, spectrumTestLayer_name);
	return true;
}

auto spectrumTestLayer_tick(mxc::ApplicationPtr appPtr, float deltaTime, void* layerData) -> mxc::ApplicationSignal_t
{
	mxc::VulkanApplication& app = appPtr.get<mxc::VulkanApplication>();
	auto spectrumTestLayerData = reinterpret_cast<SpectrumTestLayer_data*>(layerData);
	auto& renderer = app.getRenderer();
	auto* ctx = renderer.getContextPointer();
	auto& vulkanDevice = ctx->device;


	mxc::RendererStatus status = 
	renderer.recordComputeCommands([ct = spectrumTestLayerData, ctx, &app, vulkanDevice, renderer](VkCommandBuffer cmdBuf, VkImage swapchainImage, VkImageView swapchainView, uint32_t imageIndex) mutable -> VkResult 
	{
		// transition swapchain image to VK_IMAGE_LAYOUT_GENERAL (compute shader)
		// Note that I'm beginning another primary command buffer "within the recording scope" of another one. I should be able to do it as long as
		// the two of them are in 2 separate command pool
		auto& [ descriptorInfo, currentLayout ] = ct->swapchainImageInfos[imageIndex];

		vulkanDevice.insertImageMemoryBarrier(cmdBuf, swapchainImage, 
											  currentLayout, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
		currentLayout = VK_IMAGE_LAYOUT_GENERAL;

		// update descriptors with current content of the swapchain image
		//ct->shaderSet.resources.update(ctx, imageIndex, &descriptorInfo);
		descriptorInfo.imageView = swapchainView;
		renderer.fpCmdPushDescriptorSetWithTemplateKHR(cmdBuf, 
			ct->shaderSet.resources.descriptorUpdateTemplates[0],
			ct->pipeline.layout,
			0/*imageIndex*/, // set number . TODO fix this
			&descriptorInfo);

		vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, ct->pipeline.handle);

		auto [width, height] = app.getWindowExtent();
		vkCmdDispatch(cmdBuf, width / 16, height / 16, 1);

		// transition swapchain image to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR (after compute shader)
		vulkanDevice.insertImageMemoryBarrier(cmdBuf, swapchainImage, 
											  currentLayout,VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,VK_IMAGE_ASPECT_COLOR_BIT);
		currentLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		return VK_SUCCESS;
	});

	if (status == mxc::RendererStatus::FATAL)
		return mxc::ApplicationSignal_v::CLOSE_APP;
	else if (status == mxc::RendererStatus::WINDOW_RESIZED)
		MXC_WARN("Manual resize Shouldn't be triggered. Where is GLFW?");

	status = renderer.submitFrame();

	if (status == mxc::RendererStatus::FATAL)
		return mxc::ApplicationSignal_v::CLOSE_APP;
	else if (status == mxc::RendererStatus::WINDOW_RESIZED)
	{
		MXC_WARN("Manual resize Shouldn't be triggered. Where is GLFW?");
		return mxc::ApplicationSignal_v::NONE;
	}

	return mxc::ApplicationSignal_v::NONE;
}

auto spectrumTestLayer_shutdown(mxc::ApplicationPtr appPtr, void* layerData) -> void
{
	mxc::VulkanApplication& app = appPtr.get<mxc::VulkanApplication>();
	auto spectrumTestLayerData = reinterpret_cast<SpectrumTestLayer_data*>(layerData);
	auto& renderer = app.getRenderer();
	auto* ctx = renderer.getContextPointer();
	auto& vulkanDevice = ctx->device;

    spectrumTestLayerData->layoutTransitionCmdBuf.free(ctx);
	spectrumTestLayerData->pipeline.destroy(ctx);
	spectrumTestLayerData->shaderSet.destroy(ctx);
}

auto spectrumTestLayer_handler([[maybe_unused]] mxc::ApplicationPtr appPtr, [[maybe_unused]]mxc::EventName name, 
						   [[maybe_unused]] void* layerData, [[maybe_unused]]mxc::EventData eventData) -> mxc::ApplicationSignal_t
{
	mxc::VulkanApplication& app = appPtr.get<mxc::VulkanApplication>();
	auto spectrumTestLayerData = reinterpret_cast<SpectrumTestLayer_data*>(layerData);
	auto& renderer = app.getRenderer();
	auto* ctx = renderer.getContextPointer();
	auto& vulkanDevice = ctx->device;

	if (name == mxc::windowResizedEvent_name)
	{
		for (uint32_t i = 0; i != spectrumTestLayerData->swapchainImageInfos.size(); ++i)
		{
			spectrumTestLayerData->swapchainImageInfos[i].currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}
	}

	return mxc::ApplicationSignal_v::NONE;
}
