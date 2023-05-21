#include "Application.h"
#include "logging.h"

mxc::ApplicationLayer testLayer {
		.init = [](mxc::Application& app) -> bool { return true; },
		.tick = [](mxc::Application& app) -> mxc::ApplicationSignal_t { MXC_INFO("Layer works fine"); return mxc::ApplicationSignal_v::REMOVE_LAYER; },
		.shutdown = [](mxc::Application& app) -> void {},
		.handler = [](mxc::Application& app, mxc::EventName name, void* data) -> mxc::ApplicationSignal_t { return mxc::ApplicationSignal_v::NONE; },

		.data = nullptr,
};

auto initializeApplication(mxc::Application& app) -> bool
{
	app.pushLayer(testLayer, "testLayer");
	return true;
}
