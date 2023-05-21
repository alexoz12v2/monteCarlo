#include "Application.h"

#include <cstring>
#include <algorithm>

namespace mxc 
{
    auto Application::handleSignal(ApplicationSignal_t sig, uint32_t emitterLayerIndex) -> void
    {
        switch (sig)
        {
            case ApplicationSignal_v::NONE: break;
            case ApplicationSignal_v::REMOVE_LAYER: 
                m_layers[emitterLayerIndex].layer.shutdown(); 
                removeLayerEventHandlers(m_layers[emitterLayerIndex].name);
                m_layers.erase(m_layers.cbegin() + emitterLayerIndex);
                break;
            case ApplicationSignal_v::FATAL_ERROR: 
                shutdown();
                break;
            default: break;
        }
    }

    Application::Application()
    {
        m_layers.reserve(INITIAL_LAYERS_CAPACITY);
    }

    Application::~Application()
    {
        shutdown();
    }
    
    auto Application::pushLayer(ApplicationLayer const& layer, LayerName name, bool initializeNow) -> bool
    {
        m_layers.push_back({layer, name, false});
        if (initializeNow) 
        {
            if (!m_layers.back().layer.init())
            {
                m_layers.pop_back();
                return false;
            }

            else m_layers.back().isInit = true;
        }

        return true;
    }

    auto Application::init() -> bool
    {
        for (auto& layer : m_layers)
        {
            if (!layer.isInit)
            {
                // TODO how to handle errors, maybe add error handler to ApplicationLayer
                layer.layer.init();
                layer.isInit = true;
            }
        }
        return true;
    }

    auto Application::run() -> void
    {
        while (m_layers.empty())
        {
            ApplicationSignal_t sig = tick();
            if ((sig & ApplicationSignal_v::FATAL_ERROR) == ApplicationSignal_v::FATAL_ERROR) 
            {
                return;
            }
        }
    }

    auto Application::tick() -> ApplicationSignal_t
    {
        for (uint32_t i = 0; i != m_layers.size(); ++i)
        {
            if (m_layers[i].isInit)
            {
                // TODO how to handle errors, maybe add error handler to ApplicationLayer
                ApplicationSignal_t sig = m_layers[i].layer.tick();
                handleSignal(sig, i);
            }
        }

        return ApplicationSignal_v::NONE;
    }

    auto Application::emitEvent(EventName name, void* data) -> void
    {
        if (m_eventHandlers.contains(name))
        {
            std::vector<std::vector<InternalLayer>::iterator>& layerReferences = m_eventHandlers.find(name)->second;
            for (uint32_t i = 0; i != layerReferences.size(); ++i)
            {
                ApplicationSignal_t sig = layerReferences[i]->layer.handler(name, data);
                handleSignal(sig, i);
            }
        }
    }

    auto Application::registerHandler(EventName name, LayerName listener) -> void
    {
	auto findLayerByName = [listener](InternalLayer const& internalLayer) -> bool { return strcmp(internalLayer.name, listener) == 0; };
	auto findLayerRefByName = [listener](std::vector<InternalLayer>::iterator const& internalLayerRef) -> bool 
            { return strcmp(internalLayerRef->name, listener) == 0; };
        std::vector<InternalLayer>::iterator iter = std::find_if(m_layers.begin(), m_layers.end(), findLayerByName);

        // check if other listeners for the given event already exist
        if (m_eventHandlers.contains(name))
        {
            // if so, take the array of listeners
            std::vector<std::vector<InternalLayer>::iterator>& layerReferences = m_eventHandlers.find(name)->second;
            
            // and add the new listener only if it isn't already listening for the event
            if (std::find_if(layerReferences.cbegin(), layerReferences.cend(), findLayerRefByName) != layerReferences.cend())
                return;

            layerReferences.push_back(iter);
        }
        else
        {
            m_eventHandlers[name] = /*std::vector<std::vector<std::vector<InternalLayer>::iterator>>*/{iter};
        }
    }

    auto Application::shutdown() -> void
    {
        for (auto const& layer : m_layers)
        {
            if (layer.isInit)
            {
                layer.layer.shutdown();
            }
        }
    }

    auto Application::removeLayerEventHandlers(LayerName listener) -> void
    {
	auto findLayerRefByName = [listener](std::vector<InternalLayer>::iterator const& internalLayerRef) -> bool 
            { return strcmp(internalLayerRef->name, listener) == 0; };
        for (auto& pair : m_eventHandlers)
        {
            std::erase_if(pair.second, findLayerRefByName);
        }
    }
}


auto main([[maybe_unused]]int32_t argc, [[maybe_unused]]char** argv) -> int32_t
{
	mxc::Application app;

	MXC_INFO("Initializing application...");
	if (!initializeApplication(app))
	{	
		MXC_ERROR("Couldn't initialize application");
		return 1;
	}

	MXC_INFO("Running the application...");
	app.run();

	MXC_INFO("Shutting down the application...");
}

