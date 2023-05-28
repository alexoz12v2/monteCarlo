#include "Application.h"
#include "Shader.h"
#include "Pipeline.h"
#include "Renderer.h"
#include "Buffer.h"
#include "logging.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#include <fmt/core.h>
#include <X11/Xlib-xcb.h>

#include <algorithm> // std::max

struct PosColorVertex
{
    float x;
    float y;
    float z;
    uint32_t abgr;
};

// offsetof works only for standard layout classes
static_assert(std::is_standard_layout_v<PosColorVertex> && sizeof(PosColorVertex) == 16);

[[maybe_unused]] static PosColorVertex s_cubeVertices[] =
{
    {-0.5f,  0.5f,  0.5f, 0xffffffff },
    { 0.5f,  0.5f,  0.5f, 0xffffffff },
    {-0.5f, -0.5f,  0.5f, 0xffffffff },
    { 0.5f, -0.5f,  0.5f, 0xffffffff },
    {-0.5f,  0.5f, -0.5f, 0xffffffff },
    { 0.5f,  0.5f, -0.5f, 0xffffffff },
    {-0.5f, -0.5f, -0.5f, 0xffffffff },
    { 0.5f, -0.5f, -0.5f, 0xffffffff },
};

[[maybe_unused]] static const uint16_t s_cubeTriList[] =
{
    0, 1, 2,
    1, 3, 2,
    4, 6, 5,
    5, 6, 7,
    0, 2, 4,
    4, 2, 6,
    1, 5, 3,
    5, 7, 3,
    0, 4, 1,
    4, 5, 1,
    2, 3, 6,
    6, 3, 7,
};

char const* cubeTestLayer_name = "cubeTestLayer";

struct CubeTestLayer_data
{
	CubeTestLayer_data(VkDeviceSize vSize, VkDeviceSize iSize) 
		: vertexBuffer(vSize, mxc::BufferType_v::VERTEX)
		, indexBuffer(iSize, mxc::BufferType_v::INDEX)
		, stagingBuffer(std::max(vSize,iSize), mxc::BufferType_v::STAGING) {}

	mxc::Renderer renderer;
	GLFWwindow* window = nullptr;
	mxc::ShaderSet shaderSet;
	mxc::Pipeline graphicsPipeline;
	mxc::Buffer vertexBuffer, indexBuffer, stagingBuffer;

};

CubeTestLayer_data data(sizeof(s_cubeVertices), sizeof(s_cubeTriList));

auto cubeTestLayer_init(mxc::Application& app, void* layerData) -> bool;
auto cubeTestLayer_tick(mxc::Application& app, float deltaTime, void* layerData) -> mxc::ApplicationSignal_t;
auto cubeTestLayer_shutdown(mxc::Application& app, void* layerData) -> void;
auto cubeTestLayer_handler(mxc::Application& app, mxc::EventName name, void* layerData, void* eventData) -> mxc::ApplicationSignal_t;

static mxc::ApplicationLayer s_cubeTestLayer {
	.init = cubeTestLayer_init,
	.tick = cubeTestLayer_tick,
	.shutdown = cubeTestLayer_shutdown,
	.handler = cubeTestLayer_handler,

	.data = reinterpret_cast<void*>(&data)
};

auto initializeApplication(mxc::Application& app, int32_t argc, char** argv) -> bool
{
	app.pushLayer(s_cubeTestLayer, cubeTestLayer_name);
	return true;
}

// impl -----------------------------------------------------------------------

static auto keyCallback([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int32_t key, 
						[[maybe_unused]] int32_t scancode, [[maybe_unused]] int32_t action, [[maybe_unused]] int32_t mods) -> void;
auto windowResized(GLFWwindow *window, int width, int height) -> void
{
	auto cubeTestLayerData = reinterpret_cast<CubeTestLayer_data*>(glfwGetWindowUserPointer(window));

	cubeTestLayerData->renderer.onResize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

auto cubeTestLayer_init(mxc::Application& app, void* layerData) -> bool
{
	auto cubeTestLayerData = reinterpret_cast<CubeTestLayer_data*>(layerData);
	if (!glfwInit())
		return false;

	MXC_DEBUG("creating the window");
    static int32_t constexpr startWindowWidth = 640, startWindowHeight = 480;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	if (!(cubeTestLayerData->window = glfwCreateWindow(startWindowWidth, startWindowHeight, "Cube Test App", nullptr, nullptr)))
		return false;

	glfwSetKeyCallback(cubeTestLayerData->window, keyCallback);
	
    int32_t width, height;
    glfwGetFramebufferSize(cubeTestLayerData->window, &width, &height);

	// TODO platform independence
    mxc::RendererConfig config {};
    config.platformSurfaceExtensionName = "VK_KHR_xcb_surface";
    config.windowWidth = width;
    config.windowHeight = height;
#if defined(VK_USE_PLATFORM_XCB_KHR)
    config.connection = XGetXCBConnection(glfwGetX11Display());
    config.window = glfwGetX11Window(cubeTestLayerData->window);
#else
	#error "I haven't written anything else yet!"
#endif

	if (!cubeTestLayerData->renderer.init(config))
		return false;

	auto* ctx = cubeTestLayerData->renderer.getContextPointer();
	auto& vulkanDevice = cubeTestLayerData->renderer.getContextPointer()->device;

	// create shaders and pipeline --------------------------------------------
	mxc::ResourceConfiguration resConfig{};
	resConfig.poolSizes_count = 0;
	resConfig.pPoolSizes = nullptr;
	resConfig.pBindingNumbers = nullptr;

	wchar_t const* filenames[] = { SHADER_DIR L"/cubeTest.vert", SHADER_DIR L"/cubeTest.frag"};
	VkShaderStageFlagBits stageFlags[] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};
	VkVertexInputBindingDescription bindingDescriptions[] {
		{
			.binding = 0,
			.stride = sizeof(PosColorVertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		}
	};
	VkVertexInputAttributeDescription attributeDescriptions[2] {
		{
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = 0 
		},
		{
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_A8B8G8R8_USCALED_PACK32,//  VK_FORMAT_R8G8B8A8_UINT,
			.offset = offsetof(PosColorVertex, abgr)
		}
	};
	mxc::ShaderConfiguration shaderConfig{};
	shaderConfig.stage_count = 2; // vert and frag
	shaderConfig.filenames = filenames;
	shaderConfig.stageFlags = stageFlags;
	shaderConfig.attributeDescriptions_count = 2; // position and uint8_t colour (little endian)
	shaderConfig.bindingDescriptions_count = 1;
	shaderConfig.bindingDescriptions = bindingDescriptions;
	shaderConfig.attributeDescriptions = attributeDescriptions;

	if (!cubeTestLayerData->shaderSet.create(ctx, shaderConfig, resConfig))
		return false;

	bool res = cubeTestLayerData->graphicsPipeline.create(
		ctx,
		cubeTestLayerData->shaderSet,
		width, height,
		cubeTestLayerData->renderer.getRenderPass()
	);

	// create buffers ---------------------------------------------------------
	vulkanDevice.createBuffer(&cubeTestLayerData->vertexBuffer);
	vulkanDevice.createBuffer(&cubeTestLayerData->indexBuffer);
	vulkanDevice.createBuffer(&cubeTestLayerData->stagingBuffer, mxc::BufferMemoryOptions::SYSTEM_MEMORY);
	
	// upload vertex data -----------------------------------------------------
	MXC_INFO("sizeof(s_cubeVertices): %zu", sizeof(s_cubeVertices));
	vulkanDevice.copyToBuffer(s_cubeVertices, sizeof(s_cubeVertices), &cubeTestLayerData->stagingBuffer);
	vulkanDevice.copyBuffer(ctx, &cubeTestLayerData->stagingBuffer, &cubeTestLayerData->vertexBuffer);

	// upload index data ------------------------------------------------------
	vulkanDevice.copyToBuffer(s_cubeTriList, sizeof(s_cubeTriList), &cubeTestLayerData->stagingBuffer);
	vulkanDevice.copyBuffer(ctx, &cubeTestLayerData->stagingBuffer, &cubeTestLayerData->indexBuffer);

	// Set framebuffer resize callback ----------------------------------------
	glfwSetWindowUserPointer(cubeTestLayerData->window, layerData);
	glfwSetFramebufferSizeCallback(cubeTestLayerData->window, windowResized);

	if (!res)
		return false;

	return true;
}

auto cubeTestLayer_tick(mxc::Application& app, float deltaTime, void* layerData) -> mxc::ApplicationSignal_t
{
	auto cubeTestLayerData = reinterpret_cast<CubeTestLayer_data*>(layerData);
	if (glfwWindowShouldClose(cubeTestLayerData->window))
		return mxc::ApplicationSignal_v::CLOSE_APP;

	auto* ctx = cubeTestLayerData->renderer.getContextPointer();

	mxc::RendererStatus status = 
	cubeTestLayerData->renderer.recordGraphicsCommands([&ct = cubeTestLayerData, ctx](VkCommandBuffer cmdBuf) -> VkResult 
	{
		VkDeviceSize vertexBufferOffset = 0, vertexBufferStride = sizeof(PosColorVertex);
		vkCmdBindVertexBuffers2(
			cmdBuf, /*first binding*/0, /*binding count*/1, 
			&ct->vertexBuffer.handle,
			&vertexBufferOffset, 
			&ct->vertexBuffer.size, 
			&vertexBufferStride);
		vkCmdBindIndexBuffer(cmdBuf, ct->indexBuffer.handle, 0ull, VK_INDEX_TYPE_UINT16);
		vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, ct->graphicsPipeline.handle);

		VkRect2D const scissor {.offset = {0}, .extent = {.width = ctx->framebufferWidth, .height = ctx->framebufferHeight}};
		VkViewport const viewport {
			.x = 0.f, .y = 0.f,
			.width = static_cast<float>(ctx->framebufferWidth), .height = static_cast<float>(ctx->framebufferHeight),
			.minDepth = 0.f, .maxDepth = 1.f
		};
		
		vkCmdSetScissor(cmdBuf, 0/*first*/, 1/*count*/, &scissor); // note: count fixed at pipeline creation unless dynamic count.
		vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

		vkCmdDrawIndexed(
			cmdBuf,
			36,	// index count
			1,	// instance count
			0,	// first index
			0,	// vertex offset
			0 	// first instance ID
		);

		return VK_SUCCESS;
	});

	if (status == mxc::RendererStatus::FATAL)
		return mxc::ApplicationSignal_v::CLOSE_APP;
	else if (status == mxc::RendererStatus::WINDOW_RESIZED)
	{
		int32_t width, height;
		glfwGetFramebufferSize(data.window, &width, &height);
		data.renderer.onResize(static_cast<uint32_t>(width), static_cast<uint32_t>(height)); 
		return mxc::ApplicationSignal_v::NONE;
	}

	status = cubeTestLayerData->renderer.submitFrame();

	if (status == mxc::RendererStatus::FATAL)
		return mxc::ApplicationSignal_v::CLOSE_APP;
	else if (status == mxc::RendererStatus::WINDOW_RESIZED)
	{
		int32_t width, height;
		glfwGetFramebufferSize(data.window, &width, &height);
		data.renderer.onResize(static_cast<uint32_t>(width), static_cast<uint32_t>(height)); 
		return mxc::ApplicationSignal_v::NONE;
	}

	// TODO refactor in application. The application class should have control of things like vsync. the app should encapsulate platform events
	glfwWaitEventsTimeout(0.016666666f);
	return mxc::ApplicationSignal_v::NONE;
}

auto cubeTestLayer_shutdown(mxc::Application& app, void* layerData) -> void
{
	auto cubeTestLayerData = reinterpret_cast<CubeTestLayer_data*>(layerData);
	auto& renderer = cubeTestLayerData->renderer;
	auto& vulkanDevice = cubeTestLayerData->renderer.getContextPointer()->device;

	// necessary before buffers
	renderer.resetCommandBuffersForDestruction();
	vulkanDevice.destroyBuffer(&cubeTestLayerData->indexBuffer);
	vulkanDevice.destroyBuffer(&cubeTestLayerData->vertexBuffer);
	vulkanDevice.destroyBuffer(&cubeTestLayerData->stagingBuffer);

	cubeTestLayerData->graphicsPipeline.destroy(cubeTestLayerData->renderer.getContextPointer());

	cubeTestLayerData->shaderSet.destroy(cubeTestLayerData->renderer.getContextPointer());

	cubeTestLayerData->renderer.cleanup();
	glfwDestroyWindow(cubeTestLayerData->window);
	glfwTerminate();
}

auto cubeTestLayer_handler([[maybe_unused]] mxc::Application& app, [[maybe_unused]]mxc::EventName name, 
						   [[maybe_unused]] void* layerData, [[maybe_unused]]void* eventData) -> mxc::ApplicationSignal_t
{
	return mxc::ApplicationSignal_v::NONE;
}

// utitilies ------------------------------------------------------------------
static auto keyCallback([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int32_t key, 
						[[maybe_unused]] int32_t scancode, [[maybe_unused]] int32_t action, [[maybe_unused]] int32_t mods) -> void
{
}
