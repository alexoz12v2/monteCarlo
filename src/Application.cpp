#include "Application.h"

#include <cstring>
#include <algorithm>
#include <chrono>

namespace mxc 
{
    auto clamp(float value, float min, float max) -> float;

    auto Application::handleSignal(ApplicationSignal_t sig, uint32_t emitterIndex) -> bool
    {
        switch (sig)
        {
            case ApplicationSignal_v::NONE: 
                break;
            case ApplicationSignal_v::REMOVE_LAYER: 
                m_layers[emitterIndex].layer.shutdown(*this, m_layers[emitterIndex].layer.data); 
                m_layers[emitterIndex].isInit = false;
                m_removeIndices.push_back(emitterIndex);
                removeLayerEventHandlers(m_layers[emitterIndex].name);
                break;
            case ApplicationSignal_v::FATAL_ERROR: 
                return false;
            case ApplicationSignal_v::CLOSE_APP:
                m_state = ApplicationState::CLOSING;
                break;
        }
        
        return true;
    }

    Application::Application()
    {
        m_layers.reserve(INITIAL_LAYERS_CAPACITY);
        m_removeIndices.reserve(INITIAL_LAYERS_CAPACITY);
    }

    Application::~Application()
    {
        if (m_state == ApplicationState::CLOSING)
        {
            MXC_INFO("Shutting down the application...");
            shutdown();
        }
    }
    
    auto Application::pushLayer(ApplicationLayer const& layer, LayerName name, bool initializeNow) -> bool
    {
        m_layers.push_back({layer, name, false});
        MXC_TRACE("Pushing layer \"%s\"", name);
        MXC_TRACE("Now the application has %zu layers", m_layers.size());
        if (initializeNow) 
        {
            if (!m_layers.back().layer.init(*this, m_layers.back().layer.data))
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
                layer.layer.init(*this, layer.layer.data);
                layer.isInit = true;
            }
        }
        return true;
    }

    auto Application::run() -> void
    {
        std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();
        while (!m_layers.empty() && m_state == ApplicationState::OK)
        {
            float deltaTime = std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - lastTime).count();
            lastTime = std::chrono::high_resolution_clock::now();
            deltaTime = clamp(deltaTime, 1.f/500, .4);
            tick(deltaTime);
        }

        if (m_layers.empty())
        {
            m_state = ApplicationState::CLOSING;
        }
    }

    // TODO deltaTime
    auto Application::tick(float deltaTime) -> void 
    {
        for (uint32_t i = 0; i != m_layers.size(); ++i)
        {
            if (m_layers[i].isInit)
            {
                ApplicationSignal_t sig = m_layers[i].layer.tick(*this, deltaTime, m_layers[i].layer.data);
                if (!handleSignal(sig, i))
                {
                    m_state = ApplicationState::ERR;
                    return;
                }
            }
        }

        if (!m_removeIndices.empty())
        {
            // check uniqueness of indices
            std::sort(m_removeIndices.begin(), m_removeIndices.end());
            auto onePastlastUnique = std::unique(m_removeIndices.begin(), m_removeIndices.end());
            m_removeIndices.erase(onePastlastUnique, m_removeIndices.end());
            
            MXC_ASSERT(m_removeIndices.size() <= m_layers.size(), "cannot remove more layers than what you have");
            for (uint32_t i : m_removeIndices)
            {
                // {1,2,3,4,5,6,7,8,9} -> {5,6,7,8,9,1,2,3,4}
                // std::rotate(vec.begin(), vec.begin() + 4, vec.end());
                std::rotate(m_layers.begin(), m_layers.begin() + i, m_layers.end());
            }
            m_layers.erase(m_layers.end() - m_removeIndices.size(), m_layers.end());
            m_removeIndices.clear();
            MXC_TRACE("Now the application has %zu layers", m_layers.size());
        }
    }

    auto Application::emitEvent(EventName name, void* data) -> void
    {
        MXC_TRACE("Emitting event %s", name);
        if (m_eventHandlers.contains(name))
        {
            std::vector<InternalLayerPointer>& layerReferences = m_eventHandlers.find(name)->second;
            for (auto internalLayerPointer : layerReferences)
            {
                ApplicationSignal_t sig = internalLayerPointer->layer.handler(*this, name, internalLayerPointer->layer.data, data);
                uint32_t layerIndex = static_cast<uint32_t>(std::distance(m_layers.begin(), internalLayerPointer));
                if (!handleSignal(sig, layerIndex))
                {
                    m_state = ApplicationState::ERR;
                    return;
                }
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
                layer.layer.shutdown(*this, layer.layer.data);
            }
        }
    }

    auto Application::removeLayerEventHandlers(LayerName listener) -> void
    {
        auto findLayerRefByName = [listener](InternalLayerPointer const& internalLayerRef) -> bool 
        { return strcmp(internalLayerRef->name, listener) == 0; };

        // std::unordered_map<EventName, std::vector<InternalLayerPointer>>::iterator
        for (auto pairIter = m_eventHandlers.begin(); pairIter != m_eventHandlers.end(); /**/)
        {
            std::erase_if(pairIter->second, findLayerRefByName);
            if (pairIter->second.empty())
            {
                pairIter = m_eventHandlers.erase(pairIter);
            }
            else
            {
                ++pairIter;
            }
        }
    }
    
    auto clamp(float value, float min, float max) -> float
    {
        return  value < min ? min :
                value > max ? max :
                value;
    }
}

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
