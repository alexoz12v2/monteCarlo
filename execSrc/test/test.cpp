#include "Application.h"
#include "logging.h"

mxc::ApplicationLayer testLayer {
		.init = []() -> bool { return true; },
		.tick = []() -> mxc::ApplicationSignal_t { MXC_INFO("Layer works fine"); return mxc::ApplicationSignal_v::REMOVE_LAYER; },
		.shutdown = []() -> void {},
		.handler = [](mxc::EventName name, void* data) -> mxc::ApplicationSignal_t { return mxc::ApplicationSignal_v::NONE; },

		.data = nullptr,
};

auto initializeApplication(mxc::Application& app) -> bool
{
	app.pushLayer(testLayer, "testLayer");
	return true;
}
