#ifndef MXC_APPLICATION_H
#define MXC_APPLICATION_H

#include "logging.h"
#include "ApplicationPtr.h"

#include <cstdint>
#include <cstring>
#include <vector> // TODO remove
#include <string_view>
#include <unordered_map>

auto main([[maybe_unused]]int32_t argc, [[maybe_unused]]char** argv) -> int32_t;

namespace mxc
{
	class Application;
	
	// inspired from kohi game engine
	// It is a union that is 128 bits in size, meaning data can be mixed
	// and matched as required by the developer.
	struct EventData {
		// 128 bytes
		union 
		{
			void* p[2]; // to use if cannot fit
			int64_t i64[2];
			uint64_t u64[2];
			double f64[2];
			int32_t i32[4];
			uint32_t u32[4];
			float f32[4];
			int16_t i16[8];
			uint16_t u16[8];
			int8_t i8[16];
			uint8_t u8[16];
			char c[16];
		} data;
	};
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
	
	using LayerName = std::string_view;
	using EventName = std::string_view;

	using PFN_LayerInit = bool(*)(ApplicationPtr app, void* layerData);
	using PFN_LayerTick = ApplicationSignal_t(*)(ApplicationPtr app, float deltaTime, void* layerData);
	using PFN_LayerShutdown = void(*)(ApplicationPtr app, void* layerData);
	using PFN_EventHandler = ApplicationSignal_t(*)(ApplicationPtr app, EventName name, void* layerData, EventData eventData);

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
		constexpr EventHandler(PFN_EventHandler handler, LayerName layer, EventData data) : handler(handler), layer(layer), data(data) {}
		PFN_EventHandler handler;
		LayerName layer;
		EventData data;
	};
	
	class Application
	{
		static uint32_t constexpr INITIAL_LAYERS_CAPACITY = 16;
		static float constexpr MIN_DELTA_TIME = 1.f/500; 
		static float constexpr MAX_DELTA_TIME = 0.4f;

		friend auto ::main([[maybe_unused]]int32_t argc, [[maybe_unused]]char** argv) -> int32_t;
	public:
		Application(Application const&) = delete;
		Application(Application&&) = delete;
		auto operator=(Application const&) -> Application& = delete;
		auto operator=(Application&&) -> Application& = delete;

	public:
		~Application();
		// bool argument added to make future changes to add layers mid-execution
		auto pushLayer(ApplicationLayer const& layer, LayerName name, bool initializeNow = false) -> bool;
		auto emitEvent(EventName name, EventData data) -> void;
		auto registerHandler(EventName name, LayerName listener) -> void;

	private:
		struct InternalLayer
		{
			ApplicationLayer layer;
			LayerName name;
			bool isInit;
		};
		using InternalLayerPointer = std::vector<InternalLayer>::iterator;


	protected:
		Application();

		auto run() -> void;
		
		auto init() -> bool;
		auto tick(float deltaTime) -> void;
		auto shutdown() -> void;

		auto handleSignal(ApplicationSignal_t sig, uint32_t emitterLayerIndex) -> bool;
		auto removeLayerEventHandlers(LayerName name) -> void;

		auto layersEmpty() const -> bool {return m_layers.empty();}

		enum class ApplicationState {OK, ERR, CLOSING};
		ApplicationState m_state = ApplicationState::OK;

	private:
		std::unordered_map<EventName, std::vector<InternalLayerPointer>> m_eventHandlers;
		std::vector<InternalLayer> m_layers;
		std::vector<uint32_t> m_removeIndices;
	};
} // namespace mxc

#if !defined(MXC_PREVENT_APPLICATION_DEFINE_MAIN)
#define MXC_PREVENT_APPLICATION_DEFINE_MAIN

// defined by user. Adds all necessary layers to the application
auto initializeApplication(mxc::Application& app, int32_t argc, char** argv) -> bool;

auto main([[maybe_unused]]int32_t argc, [[maybe_unused]]char** argv) -> int32_t
{
    mxc::Application app;

    MXC_INFO("Initializing application...");
    if (!initializeApplication(app, argc, argv) || !app.init())
    {	
        MXC_ERROR("Couldn't initialize application");
        return 1;
    }

    MXC_INFO("Running the application...");
    app.run();
}
#endif // MXC_PREVENT_APPLICATION_DEFINE_MAIN
#endif // MXC_APPLICATION_H
