#include "Application.h"
#include "logging.h"

char const* testLayer_name = "testLayer";

struct TestLayer_data
{
	char const* str = "layer works fine";
};

struct PrintHello_data
{
	char const* str;
};

TestLayer_data data;

auto testLayer_init(mxc::Application& app, void* layerData) -> bool
{
	auto testLayerData = reinterpret_cast<TestLayer_data*>(layerData);
	app.registerHandler("PrintHello", testLayer_name);
	return true;
}

auto testLayer_tick(mxc::Application& app, float deltaTime, void* layerData) -> mxc::ApplicationSignal_t
{
	auto testLayerData = reinterpret_cast<TestLayer_data*>(layerData);
	MXC_INFO(testLayerData->str);

	PrintHello_data eventData {.str = "wow, events!"};
	app.emitEvent("PrintHello", reinterpret_cast<void*>(&eventData));

	return mxc::ApplicationSignal_v::REMOVE_LAYER;
}

auto testLayer_shutdown(mxc::Application& app, void* layerData) -> void
{
	auto testLayerData = reinterpret_cast<TestLayer_data*>(layerData);
}

auto testLayer_handler(mxc::Application& app, mxc::EventName name, void* layerData, void* eventData) -> mxc::ApplicationSignal_t
{
	auto testLayerData = reinterpret_cast<TestLayer_data*>(layerData);
	MXC_INFO("from the event, %s", testLayerData->str);

	// handlers
	if (mxc::Application::isEvent(name, "PrintHello"))
	{
		auto printHelloData = reinterpret_cast<PrintHello_data*>(eventData);
		MXC_DEBUG(printHelloData->str);
	}

	return mxc::ApplicationSignal_v::NONE;
}

mxc::ApplicationLayer testLayer {
	.init = testLayer_init,
	.tick = testLayer_tick,
	.shutdown = testLayer_shutdown,
	.handler = testLayer_handler,

	.data = reinterpret_cast<void*>(&data)
};

mxc::ApplicationLayer printHelloEmitterLayer {
	.init = [](mxc::Application& app, void* layerData) -> bool { return true; },
	.tick = [](mxc::Application& app, float deltaTime, void* layerData) -> mxc::ApplicationSignal_t
	{
		PrintHello_data eventData {.str = "emitting events..."};
		app.emitEvent("PrintHello", reinterpret_cast<void*>(&eventData)); 
		return mxc::ApplicationSignal_v::NONE;
	},
	.shutdown = [](mxc::Application& app, void* layerData) {MXC_ERROR("Shutting down the printHelloEmitterLayer");},
	.handler = [](mxc::Application& app, mxc::EventName name, void* layerData, void* eventData) -> mxc::ApplicationSignal_t 
	{ return mxc::ApplicationSignal_v::NONE; },

	.data = nullptr
};

auto initializeApplication(mxc::Application& app, int32_t argc, char** argv) -> bool
{
	app.pushLayer(printHelloEmitterLayer, "printHelloEmitterLayer");
	app.pushLayer(testLayer, testLayer_name);
	return true;
}
