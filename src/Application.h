#ifndef MXC_APPLICATION_H
#define MXC_APPLICATION_H

#include "logging.h"

#include <cstdint>
#include <cstring>
#include <vector> // TODO remove
#include <unordered_map>

auto main([[maybe_unused]]int32_t argc, [[maybe_unused]]char** argv) -> int32_t;

namespace mxc
{
	class Application;

	// setup as a bitfield so that I can change the application loop in the future using it
	namespace ApplicationSignal_v
	{	
		enum T 
		{
			NONE = 0,
			REMOVE_LAYER = 1,
			FATAL_ERROR = 1<<1
		};
	}
	using ApplicationSignal_t = ApplicationSignal_v::T;
	
	using LayerName = char const*;
	using EventName = char const*;

	using PFN_LayerInit = bool(*)(Application& app);
	using PFN_LayerTick = ApplicationSignal_t(*)(Application& app);
	using PFN_LayerShutdown = void(*)(Application& app);
	using PFN_EventHandler = ApplicationSignal_t(*)(Application& app, EventName name, void* data);

	// TODO define tags and metadata for layers to have priority execution. 
	// TODO It could also be multithreaded. 
	// TODO how about inter layer communication?
	struct ApplicationLayer
	{
		PFN_LayerInit init;
		PFN_LayerTick tick;
		PFN_LayerShutdown shutdown;
		PFN_EventHandler handler;

		void* data;
	};

	struct EventHandler
	{
		constexpr EventHandler(PFN_EventHandler handler, LayerName layer, void* data) : handler(handler), layer(layer), data(data) {}
		PFN_EventHandler handler;
		LayerName layer;
		void* data;
	};
	
	class Application
	{
		static uint32_t constexpr INITIAL_LAYERS_CAPACITY = 16;

		friend auto ::main([[maybe_unused]]int32_t argc, [[maybe_unused]]char** argv) -> int32_t;
	public:
		~Application();
		// bool argument added to make future changes to add layers mid-execution
		auto pushLayer(ApplicationLayer const& layer, LayerName name, bool initializeNow = false) -> bool;
		auto emitEvent(EventName name, void* data) -> void;
		auto registerHandler(EventName name, LayerName listener) -> void;

	private:
		Application();

		auto run() -> void;
		
		auto init() -> bool;
		auto tick() -> ApplicationSignal_t;
		auto shutdown() -> void;

		auto handleSignal(ApplicationSignal_t sig, uint32_t emitterLayerIndex) -> void;
		auto removeLayerEventHandlers(LayerName name) -> void;

		struct InternalLayer
		{
			ApplicationLayer layer;
			LayerName name;
			bool isInit;
		};

		std::vector<InternalLayer> m_layers;
		std::unordered_map<EventName, std::vector<std::vector<InternalLayer>::iterator>> m_eventHandlers;
	};
}

// defined by user. Adds all necessary layers to the application
auto initializeApplication(mxc::Application& app) -> bool;

#endif
