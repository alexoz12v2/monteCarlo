#include "VulkanApplication.h"
#include "Shader.h"
#include "Pipeline.h"
#include "Renderer.h"
#include "Buffer.h"
#include "logging.h"

#include <vector>
#include <cmath>
#include <random>

static auto constexpr spectrumTestLayer_name = "spectrumTestLayer";

struct SpectrumTestLayer_data
{
	mxc::ShaderSet shaderSet;
	mxc::Pipeline pipeline;
	// TODO make as many as swapchain Images
	// TODO remove vector
	struct ImageInfo { VkDescriptorImageInfo descriptorInfo; VkImageLayout currentLayout;};
	std::vector<ImageInfo> swapchainImageInfos;
	std::vector<VkDescriptorImageInfo> transactionImageInfos;
	std::vector<mxc::Image> transactionImages;
	std::vector<mxc::ImageView> transactionImageViews;
	mxc::CommandBuffer layoutTransitionCmdBuf;
	uint32_t samplesPerPixel;
	uint32_t sampleIndex;
	bool usePushDescriptors;
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
		{.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 2}
	};
	uint32_t const bindingNumbers_counts[POOLSIZES_COUNT] { 2 };
	uint32_t const bindingNumbers[] { 0, 1 };
	VkPushConstantRange pushConstantRange{ .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .offset = 0, .size = 3*sizeof(uint32_t) };

	mxc::ResourceConfiguration resConfig{};
	resConfig.poolSizes_count = POOLSIZES_COUNT;
	resConfig.pPoolSizes = poolSizes;
	resConfig.pBindingNumbers = bindingNumbers;
	resConfig.pBindingNumbers_counts = bindingNumbers_counts;
	resConfig.usePushDescriptors = spectrumTestLayerData->usePushDescriptors = false;

	wchar_t const shaderDir[] = L"" SHADER_DIR;
	wchar_t const* filenames[] = { SHADER_DIR L"/spectrumTest.comp" }; // TODO: filenames relative to shaderDir
	VkShaderStageFlagBits stageFlags[] = {VK_SHADER_STAGE_COMPUTE_BIT}; // as many as layouts i.e., one
	mxc::ShaderConfiguration shaderConfig{};
	shaderConfig.stage_count = 1;
	shaderConfig.filenames = filenames;
	shaderConfig.stageFlags = stageFlags;
	shaderConfig.shaderDir = shaderDir;

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
		width, height,
		VK_NULL_HANDLE, // renderpass = null -> compute pipeline
		&pushConstantRange, 1
	);

	if (!res)
		return false;

	// create descriptor sets update template ---------------------------------
	for (uint32_t i = 0; i != 1; ++i)
	{	
		VkImageLayout constexpr targetLayout = VK_IMAGE_LAYOUT_GENERAL;
		spectrumTestLayerData->transactionImages.push_back({ 
			VK_IMAGE_TYPE_2D, 
			VkExtent3D{.width = width, .height = height, .depth = 1},
			ctx->swapchain.imageFormat.format,
			VK_IMAGE_USAGE_STORAGE_BIT
		});	

		spectrumTestLayerData->transactionImageViews.push_back({ VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
		spectrumTestLayerData->transactionImageInfos.push_back(VkDescriptorImageInfo{
			.sampler = VK_NULL_HANDLE, // only used for descriptors of type sampler and combined image samplers
			.imageView = spectrumTestLayerData->transactionImageViews[i].handle,
			.imageLayout = targetLayout
		});

		vulkanDevice.createImage(
			ctx, 
			VK_IMAGE_TILING_OPTIMAL, 
			&spectrumTestLayerData->transactionImages[i], 
			&targetLayout, 
			&spectrumTestLayerData->transactionImageViews[i],
			mxc::CommandType::GRAPHICS);
		MXC_ASSERT(spectrumTestLayerData->transactionImageViews[i].handle != VK_NULL_HANDLE, "transactionImageViews[%u].handle is null", i);
		spectrumTestLayerData->transactionImageInfos[i].imageView = spectrumTestLayerData->transactionImageViews[i].handle;
	}

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
		vulkanDevice.insertImageMemoryBarrier(cmdBuf.handle, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,//PRESENT_SRC_KHR,
			VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	}
	cmdBuf.end();
	vulkanDevice.flushCommandBuffer(&cmdBuf, mxc::CommandType::GRAPHICS);
	cmdBuf.free(ctx);

	// Other Variables --------------------------------------------------------
	spectrumTestLayerData->samplesPerPixel = 25000;
	spectrumTestLayerData->sampleIndex = 0;
	
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

    static std::random_device r;
    static std::mt19937 e1(r());
    static std::uniform_int_distribution<uint32_t> uniformDist(1, UINT32_MAX);

	if (!spectrumTestLayerData->ready)
	{
		spectrumTestLayerData->ready = true;
		return mxc::ApplicationSignal_v::NONE;
	}

	uint32_t* outImageIndex = nullptr;
	mxc::RendererStatus status = 
	renderer.recordComputeCommands([ct = spectrumTestLayerData, ctx, &app, vulkanDevice, renderer, outImageIndex]
	(VkCommandBuffer cmdBuf, VkImage swapchainImage, VkImageView swapchainView, uint32_t imageIndex) mutable -> VkResult 
	{
		outImageIndex = &imageIndex;
		VkCommandBuffer drawCmdBuf = ctx->syncObjs[imageIndex].commandBuffer;
		auto [width, height] = app.getWindowExtent();
		auto& [ descriptorInfo, currentLayout ] = ct->swapchainImageInfos[imageIndex];
		auto& transactionDescriptorInfo = ct->transactionImageInfos[0];
		currentLayout = VK_IMAGE_LAYOUT_GENERAL;
		descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		descriptorInfo.imageView = swapchainView;

		MXC_ASSERT(transactionDescriptorInfo.imageView != VK_NULL_HANDLE 
				   && transactionDescriptorInfo.imageLayout == VK_IMAGE_LAYOUT_GENERAL, "transaction image info is not valid");
		MXC_ASSERT(descriptorInfo.imageView != VK_NULL_HANDLE 
				   && descriptorInfo.imageLayout == VK_IMAGE_LAYOUT_GENERAL, "transaction image info is not valid");
		MXC_ASSERT(renderer.fpCmdPushDescriptorSetWithTemplateKHR, "function pointer for push descriptors is nullptr");

		// update descriptors with current content of the swapchain image
		VkDescriptorImageInfo const thing[] = { descriptorInfo, transactionDescriptorInfo };
		if (ct->usePushDescriptors)
			renderer.fpCmdPushDescriptorSetWithTemplateKHR(cmdBuf, 
				ct->shaderSet.resources.descriptorUpdateTemplates[0],
				ct->pipeline.layout,
				0, // set number
				thing);
		else
		{
			ct->shaderSet.resources.update(ctx, imageIndex, thing);
			vkCmdBindDescriptorSets(
				cmdBuf,
				VK_PIPELINE_BIND_POINT_COMPUTE,
				ct->pipeline.layout,
				0/*firstSet*/,
				1/*descriptorSetCount*/,
				&ct->shaderSet.resources.descriptorSets[imageIndex],
				0/*dynamicOffsetCount*/,
				nullptr/*pDynamicOffsets*/);
		}

		uint32_t rndSeed = uniformDist(e1);
		uint32_t samplesIndex = ct->sampleIndex++;
		uint32_t pushVar[] = { rndSeed, samplesIndex, ct->samplesPerPixel };
		vkCmdPushConstants(
			cmdBuf,
			ct->pipeline.layout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			0,
			3*sizeof(uint32_t),
			&pushVar);

		vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, ct->pipeline.handle);

		vkCmdDispatch(cmdBuf, static_cast<uint32_t>(ceil(width / 16.f)), static_cast<uint32_t>(ceil(height / 16.f)), 1);

		return VK_SUCCESS;
	});

	status = renderer.submitCompute(true);

	if (status == mxc::RendererStatus::FATAL)
		return mxc::ApplicationSignal_v::CLOSE_APP;

	++loopCounter;
	return mxc::ApplicationSignal_v::NONE;
}

auto spectrumTestLayer_shutdown(mxc::ApplicationPtr appPtr, void* layerData) -> void
{
	mxc::VulkanApplication& app = appPtr.get<mxc::VulkanApplication>();
	auto spectrumTestLayerData = reinterpret_cast<SpectrumTestLayer_data*>(layerData);
	auto& renderer = app.getRenderer();
	auto* ctx = renderer.getContextPointer();
	auto& vulkanDevice = ctx->device;

	for (auto& view : spectrumTestLayerData->transactionImageViews)
		vulkanDevice.destroyImageView(&view);

	for (auto& image : spectrumTestLayerData->transactionImages)
		vulkanDevice.destroyImage(&image);

    spectrumTestLayerData->layoutTransitionCmdBuf.free(ctx);
	spectrumTestLayerData->pipeline.destroy(ctx);
	spectrumTestLayerData->shaderSet.destroy(ctx);
}

auto spectrumTestLayer_handler(mxc::ApplicationPtr appPtr, mxc::EventName name, 
							   void* layerData, mxc::EventData eventData) -> mxc::ApplicationSignal_t
{
	mxc::VulkanApplication& app = appPtr.get<mxc::VulkanApplication>();
	auto spectrumTestLayerData = reinterpret_cast<SpectrumTestLayer_data*>(layerData);
	auto& renderer = app.getRenderer();
	auto* ctx = renderer.getContextPointer();
	auto& vulkanDevice = ctx->device;

	if (name == mxc::windowResizedEvent_name)
	{
		uint32_t width = eventData.data.u32[0], height = eventData.data.u32[1];
		MXC_WARN("I Should be running after Renderer::Resize");
		spectrumTestLayerData->ready = false;
		spectrumTestLayerData->sampleIndex = 0;
		for (uint32_t i = 0; i != spectrumTestLayerData->swapchainImageInfos.size(); ++i)
		{
			spectrumTestLayerData->swapchainImageInfos[i].currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			spectrumTestLayerData->swapchainImageInfos[i].descriptorInfo.imageView = ctx->swapchain.images[i].view;
			spectrumTestLayerData->swapchainImageInfos[i].descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}

		// transition swapchain images to VK_LAYOUT_GENERAL -----------------------
		mxc::CommandBuffer cmdBuf;
		cmdBuf.allocate(ctx, mxc::CommandType::GRAPHICS);
		cmdBuf.begin();
		for (uint32_t i = 0; i != ctx->swapchain.images.size(); ++i)
			vulkanDevice.insertImageMemoryBarrier(cmdBuf.handle, ctx->swapchain.images[i].handle, 
											VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
		cmdBuf.end();
		vulkanDevice.flushCommandBuffer(&cmdBuf, mxc::CommandType::GRAPHICS);
		cmdBuf.free(ctx);

		// destroy and recreate transaction image
		for (auto& view : spectrumTestLayerData->transactionImageViews)
			vulkanDevice.destroyImageView(&view);

		for (auto& image : spectrumTestLayerData->transactionImages)
			vulkanDevice.destroyImage(&image);

		spectrumTestLayerData->transactionImageViews.clear();
		spectrumTestLayerData->transactionImages.clear();
		for (uint32_t i = 0; i != 1; ++i)
		{	
			spectrumTestLayerData->transactionImages.push_back({ 
				VK_IMAGE_TYPE_2D, 
				VkExtent3D{.width = width, .height = height, .depth = 1},
				ctx->swapchain.imageFormat.format,
				VK_IMAGE_USAGE_STORAGE_BIT
			});	

			spectrumTestLayerData->transactionImageViews.push_back({ VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT });
			spectrumTestLayerData->transactionImageInfos.push_back({
				.sampler = VK_NULL_HANDLE, // only used for descriptors of type sampler and combined image samplers
				.imageView = spectrumTestLayerData->transactionImageViews[i].handle,
				.imageLayout = VK_IMAGE_LAYOUT_GENERAL
			});

			VkImageLayout constexpr targetLayout = VK_IMAGE_LAYOUT_GENERAL;
			vulkanDevice.createImage(
				ctx, 
				VK_IMAGE_TILING_OPTIMAL, 
				&spectrumTestLayerData->transactionImages[i], 
				&targetLayout, 
				&spectrumTestLayerData->transactionImageViews[i],
				mxc::CommandType::GRAPHICS);

			spectrumTestLayerData->transactionImageInfos[i].imageView = spectrumTestLayerData->transactionImageViews[i].handle;
		}
	}

	return mxc::ApplicationSignal_v::NONE;
}
