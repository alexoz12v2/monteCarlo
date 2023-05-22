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
			FATAL_ERROR = 1<<1,
			CLOSE_APP = 1<<2
		};
	}
	using ApplicationSignal_t = ApplicationSignal_v::T;
	
	using LayerName = char const*;
	using EventName = char const*;

	using PFN_LayerInit = bool(*)(Application& app, void* layerData);
	using PFN_LayerTick = ApplicationSignal_t(*)(Application& app, float deltaTime, void* layerData);
	using PFN_LayerShutdown = void(*)(Application& app, void* layerData);
	using PFN_EventHandler = ApplicationSignal_t(*)(Application& app, EventName name, void* layerData, void* eventData);

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
		static float constexpr MIN_DELTA_TIME = 1.f/500; 
		static float constexpr MAX_DELTA_TIME = 0.4f;

		friend auto ::main([[maybe_unused]]int32_t argc, [[maybe_unused]]char** argv) -> int32_t;
	public:
		~Application();
		// bool argument added to make future changes to add layers mid-execution
		auto pushLayer(ApplicationLayer const& layer, LayerName name, bool initializeNow = false) -> bool;
		auto emitEvent(EventName name, void* data) -> void;
		auto registerHandler(EventName name, LayerName listener) -> void;

		static inline auto isEvent(EventName x, EventName y) -> bool { return strcmp(x,y) == 0; }

	private:
		struct InternalLayer
		{
			ApplicationLayer layer;
			LayerName name;
			bool isInit;
		};
		using InternalLayerPointer = std::vector<InternalLayer>::iterator;

		enum class ApplicationState {OK, ERR, CLOSING};

	private:
		Application();

		auto run() -> void;
		
		auto init() -> bool;
		auto tick(float deltaTime) -> void;
		auto shutdown() -> void;

		auto handleSignal(ApplicationSignal_t sig, uint32_t emitterLayerIndex) -> bool;
		auto removeLayerEventHandlers(LayerName name) -> void;

		std::unordered_map<EventName, std::vector<InternalLayerPointer>> m_eventHandlers;
		std::vector<InternalLayer> m_layers;
		std::vector<uint32_t> m_removeIndices;
		ApplicationState m_state = ApplicationState::OK;
	};
}

// defined by user. Adds all necessary layers to the application
auto initializeApplication(mxc::Application& app, int32_t argc, char** argv) -> bool;

#endif
