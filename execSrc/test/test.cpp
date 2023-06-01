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

auto testLayer_init(mxc::ApplicationPtr appPtr, void* layerData) -> bool
{
	auto& app = appPtr.get<mxc::Application>();
	auto testLayerData = reinterpret_cast<TestLayer_data*>(layerData);
	app.registerHandler("PrintHello", testLayer_name);
	return true;
}

auto testLayer_tick(mxc::ApplicationPtr appPtr, float deltaTime, void* layerData) -> mxc::ApplicationSignal_t
{
	auto& app = appPtr.get<mxc::Application>();
	auto testLayerData = reinterpret_cast<TestLayer_data*>(layerData);
	MXC_INFO(testLayerData->str);

	PrintHello_data eventData {.str = "wow, events!"};
	mxc::EventData evData{};
	evData.data.p[0] = reinterpret_cast<void*>(&eventData);
	app.emitEvent("PrintHello", evData);

	return mxc::ApplicationSignal_v::REMOVE_LAYER;
}

auto testLayer_shutdown(mxc::ApplicationPtr appPtr, void* layerData) -> void
{
	auto& app = appPtr.get<mxc::Application>();
	auto testLayerData = reinterpret_cast<TestLayer_data*>(layerData);
}

auto testLayer_handler(mxc::ApplicationPtr appPtr, mxc::EventName name, void* layerData, mxc::EventData eventData) -> mxc::ApplicationSignal_t
{
	auto& app = appPtr.get<mxc::Application>();
	auto testLayerData = reinterpret_cast<TestLayer_data*>(layerData);
	MXC_INFO("from the event, %s", testLayerData->str);

	// handlers
	if (name == "PrintHello")
	{
		auto printHelloData = reinterpret_cast<PrintHello_data*>(eventData.data.p[0]);
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
	.init = [](mxc::ApplicationPtr appPtr, void* layerData) -> bool { return true; },
	.tick = [](mxc::ApplicationPtr appPtr, float deltaTime, void* layerData) -> mxc::ApplicationSignal_t
	{
		auto& app = appPtr.get<mxc::Application>();
		PrintHello_data eventData {.str = "emitting events..."};
		mxc::EventData evData{};
		evData.data.p[0] = reinterpret_cast<void*>(&eventData);
		app.emitEvent("PrintHello", evData); 
		return mxc::ApplicationSignal_v::NONE;
	},
	.shutdown = [](mxc::ApplicationPtr appPtr, void* layerData) {MXC_ERROR("Shutting down the printHelloEmitterLayer");},
	.handler = [](mxc::ApplicationPtr appPtr, mxc::EventName name, void* layerData, mxc::EventData eventData) -> mxc::ApplicationSignal_t 
	{ return mxc::ApplicationSignal_v::NONE; },

	.data = nullptr
};

auto initializeApplication(mxc::Application& app, int32_t argc, char** argv) -> bool
{
	app.pushLayer(printHelloEmitterLayer, "printHelloEmitterLayer");
	app.pushLayer(testLayer, testLayer_name);
	return true;
}
