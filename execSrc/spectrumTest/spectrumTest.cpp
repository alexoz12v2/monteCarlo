#include "VulkanApplication.h"
#include "Shader.h"
#include "Pipeline.h"
#include "Renderer.h"
#include "Buffer.h"
#include "logging.h"

#include <vector>
#include <cmath>

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
	bool ready;
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

	spectrumTestLayerData->ready = true;

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
	spectrumTestLayerData->shaderSet.resources.createUpdateTemplate(ctx,VK_PIPELINE_BIND_POINT_COMPUTE,spectrumTestLayerData->pipeline.layout,strides);

	// create buffers ---------------------------------------------------------
    //MXC_ASSERT(spectrumTestLayerData->layoutTransitionCmdBuf.allocate(ctx, mxc::CommandType::GRAPHICS), 
	//		   "Couldn't allocate image memory barrier command buffer");

	app.registerHandler(mxc::windowResizedEvent_name, spectrumTestLayer_name);

	// set extent function pointer for renderer -------------------------------
	renderer.fpGetSurfaceFramebufferSize = [&app](){
		MXC_TRACE("RESIZING CALLBACK USED");
		auto [width, height] = app.getWindowExtent();
		return VkExtent2D{width, height};
	};

	// transition swapchain images to VK_LAYOUT_GENERAL -----------------------
	mxc::CommandBuffer cmdBuf;
	cmdBuf.allocate(ctx, mxc::CommandType::GRAPHICS);
	cmdBuf.begin();
	for (uint32_t i = 0; i != ctx->swapchain.images.size(); ++i)
	{
		VkImage image = ctx->swapchain.images[i].handle;
		vulkanDevice.insertImageMemoryBarrier(cmdBuf.handle, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

		// release to compute queue if necessary 
		if (ctx->device.queueFamilies.graphics != ctx->device.queueFamilies.compute)
		{
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.dstAccessMask = 0;
			imageMemoryBarrier.srcQueueFamilyIndex = ctx->device.queueFamilies.graphics;
			imageMemoryBarrier.dstQueueFamilyIndex = ctx->device.queueFamilies.compute;
			MXC_WARN("w2");
		}
	}
	cmdBuf.end();
	vulkanDevice.flushCommandBuffer(&cmdBuf, mxc::CommandType::GRAPHICS);
	cmdBuf.free(ctx);

	return true;
}

auto spectrumTestLayer_tick(mxc::ApplicationPtr appPtr, float deltaTime, void* layerData) -> mxc::ApplicationSignal_t
{
	static uint32_t loopCounter = 0;
	mxc::VulkanApplication& app = appPtr.get<mxc::VulkanApplication>();
	auto spectrumTestLayerData = reinterpret_cast<SpectrumTestLayer_data*>(layerData);
	auto& renderer = app.getRenderer();
	auto* ctx = renderer.getContextPointer();
	auto& vulkanDevice = ctx->device;

	if (!spectrumTestLayerData->ready)
	{
		spectrumTestLayerData->ready = true;
		return mxc::ApplicationSignal_v::NONE;
	}

	uint32_t* outImageIndex = nullptr;
	MXC_WARN("-%u-", loopCounter);
	mxc::RendererStatus status = 
	renderer.recordComputeCommands([ct = spectrumTestLayerData, ctx, &app, vulkanDevice, renderer, outImageIndex]
	(VkCommandBuffer cmdBuf, VkImage swapchainImage, VkImageView swapchainView, uint32_t imageIndex) mutable -> VkResult 
	{
		outImageIndex = &imageIndex;
		VkCommandBuffer drawCmdBuf = ctx->syncObjs[imageIndex].commandBuffer;
		auto [width, height] = app.getWindowExtent();
		auto& [ descriptorInfo, currentLayout ] = ct->swapchainImageInfos[imageIndex];

		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageMemoryBarrier.image = swapchainImage;
		imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		if (ctx->device.queueFamilies.graphics != ctx->device.queueFamilies.compute)
		{
			// Acquire barrier for compute queue
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.srcQueueFamilyIndex = ctx->device.queueFamilies.graphics;
			imageMemoryBarrier.dstQueueFamilyIndex = ctx->device.queueFamilies.compute;
			vkCmdPipelineBarrier(
				cmdBuf,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0,//VK_FLAGS_NONE,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
			MXC_TRACE("Swapchain image %u acquired by compute queue", imageIndex);
		}

		currentLayout = VK_IMAGE_LAYOUT_GENERAL;
		descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		// update descriptors with current content of the swapchain image
		//ct->shaderSet.resources.update(ctx, imageIndex, &descriptorInfo);
		descriptorInfo.imageView = swapchainView;
		renderer.fpCmdPushDescriptorSetWithTemplateKHR(cmdBuf, 
			ct->shaderSet.resources.descriptorUpdateTemplates[0],
			ct->pipeline.layout,
			0/*imageIndex*/, // set number . TODO fix this
			&descriptorInfo);

		vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, ct->pipeline.handle);

		vkCmdDispatch(cmdBuf, static_cast<uint32_t>(ceil(width / 16.f)), static_cast<uint32_t>(ceil(height / 16.f)), 1);

		if (ctx->device.queueFamilies.graphics != ctx->device.queueFamilies.compute)
		{
			// Release barrier from compute queue
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.dstAccessMask = 0;
			imageMemoryBarrier.srcQueueFamilyIndex = ctx->device.queueFamilies.compute;
			imageMemoryBarrier.dstQueueFamilyIndex = ctx->device.queueFamilies.graphics;
			vkCmdPipelineBarrier(
				cmdBuf,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0,//VK_FLAGS_NONE,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
			MXC_TRACE("Swapchain image %u released by compute queue", imageIndex);
		}

		return VK_SUCCESS;
	});

	MXC_WARN("--");
	++loopCounter;
	if (status == mxc::RendererStatus::FATAL)
		return mxc::ApplicationSignal_v::CLOSE_APP;
	else if (status == mxc::RendererStatus::WINDOW_RESIZED)
		return mxc::ApplicationSignal_v::NONE;

	status = renderer.submitCompute();

	MXC_WARN("--++");
	status = renderer.transitionAndpresentFrame(VK_IMAGE_LAYOUT_GENERAL);

	if (status == mxc::RendererStatus::FATAL)
		return mxc::ApplicationSignal_v::CLOSE_APP;
	else if (status == mxc::RendererStatus::WINDOW_RESIZED)
		return mxc::ApplicationSignal_v::NONE;

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
		MXC_WARN("I Should be running after Renderer::Resize");
		spectrumTestLayerData->ready = false;
		for (uint32_t i = 0; i != spectrumTestLayerData->swapchainImageInfos.size(); ++i)
		{
			spectrumTestLayerData->swapchainImageInfos[i].currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			spectrumTestLayerData->swapchainImageInfos[i].descriptorInfo.imageView = ctx->swapchain.images[i].view;
			spectrumTestLayerData->swapchainImageInfos[i].descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}

		// transition swapchain images to VK_LAYOUT_GENERAL -----------------------
		mxc::CommandBuffer cmdBuf;
		cmdBuf.allocate(ctx, mxc::CommandType::COMPUTE);
		cmdBuf.begin();
		for (uint32_t i = 0; i != ctx->swapchain.images.size(); ++i)
			vulkanDevice.insertImageMemoryBarrier(cmdBuf.handle, ctx->swapchain.images[i].handle, 
											VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
		cmdBuf.end();
		vulkanDevice.flushCommandBuffer(&cmdBuf, mxc::CommandType::COMPUTE);
		cmdBuf.free(ctx);
	}

	return mxc::ApplicationSignal_v::NONE;
}
