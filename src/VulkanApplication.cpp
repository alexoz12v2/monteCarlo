#define MXC_PREVENT_VULKAN_APPLICATION_DEFINE_MAIN
#include "VulkanApplication.h"
#include "logging.h"

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <windows.h>
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#define GLFW_EXPOSE_NATIVE_X11
#include <X11/Xlib-xcb.h>
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#define GLFW_EXPOSE_NATIVE_WAYLAND
#include <wayland-client.h>
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
#define GLFW_EXPOSE_NATIVE_COCOA
#else
#error "why are you here"
#endif

#include <GLFW/glfw3native.h>

#include <cstring>
#include <chrono>

namespace mxc 
{
    
    auto keyCallback([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int32_t key, 
                     [[maybe_unused]] int32_t scancode, [[maybe_unused]] int32_t action, [[maybe_unused]] int32_t mods) -> void;

    auto windowResized(GLFWwindow *window, int width, int height) -> void;

    // TODO copied from Application. Find a better way to do this. BTW, not calling super's method because it calls Application::tick
    auto VulkanApplication::run() -> void
    {
        std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();
        while (!glfwWindowShouldClose(m_window) && !layersEmpty() && m_state == ApplicationState::OK)
        {
            float deltaTime = std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - lastTime).count();
            lastTime = std::chrono::high_resolution_clock::now();
            deltaTime = clamp(deltaTime, 1.f/500, .4);

            tick(deltaTime);

	    glfwWaitEventsTimeout(0.016666666f);
        }
	MXC_INFO("--- Closing The Window ---");

        if (layersEmpty())
        {
            m_state = ApplicationState::CLOSING;
        }
    }

    auto VulkanApplication::tick(float deltaTime) -> void
    {
        // logic
        Application::tick(deltaTime);
        // logic
    }
    
    auto VulkanApplication::init() -> bool
    {
	MXC_TRACE("VulkanApplication::init");
        
	if (!glfwInit())
	{
	    MXC_ERROR("Failed to initialize the GLFW library");
	    return false;
	}
        MXC_DEBUG("creating the window...");
        static int32_t constexpr startWindowWidth = 640, startWindowHeight = 480;
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        if (!(m_window = glfwCreateWindow(startWindowWidth, startWindowHeight, "Cube Test App", nullptr, nullptr)))
        {
	    MXC_ERROR("failed to create the window! pointer: %p", m_window);
            return false;
        }
        MXC_INFO("window successfully created.");

        glfwSetKeyCallback(m_window, keyCallback);
        
        int32_t width, height;
        glfwGetFramebufferSize(m_window, &width, &height);

        // Set framebuffer resize callback ----------------------------------------
        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, windowResized);

        mxc::RendererConfig config{};
        config.windowWidth = width;
        config.windowHeight = height;
#if defined(VK_USE_PLATFORM_XCB_KHR)
        config.platformSurfaceExtensionName = "VK_KHR_xcb_surface";
        config.connection = XGetXCBConnection(glfwGetX11Display());
        config.window = glfwGetX11Window(m_window);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
        config.platformSurfaceExtensionName = "VK_KHR_win32_surface";
        config.platformHandle = GetModuleHandle(nullptr);
        config.platformWindow = glfwGetWin32Window(m_window);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        config.platformSurfaceExtensionName = "VK_KHR_wayland_surface";
        config.display = glfwGetWaylandDisplay();
        config.window = glfwGetWaylandWindow(m_window);
#elif defined(VK_USE_PLATFORM_COCOA_MVK)
        config.platformSurfaceExtensionName = "VK_EXT_metal_surface";
        config.view = glfwGetCocoaWindow(m_window);
#else
        #error "I haven't written anything else yet!"
#endif

	MXC_DEBUG("creating the renderer");
    	if (!m_renderer.init(config))
	{
	    MXC_ERROR("Failed to initialize the renderer");
	    return false;
	}
	MXC_INFO("renderer successfully created");

	MXC_DEBUG("initializing application layers");
        if (!Application::init())
	{
	    MXC_ERROR("Failed to initialize application layers");
            return false;
	}
	MXC_INFO("Application layers initialized");

        return true;
    }

    auto VulkanApplication::shutdown() -> void
    {
        // necessary before buffers
        m_renderer.resetCommandBuffersForDestruction();

        m_renderer.cleanup();
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    auto VulkanApplication::handleSignal(ApplicationSignal_t sig, uint32_t emitterLayerIndex) -> bool
    {
        return Application::handleSignal(sig, emitterLayerIndex);
    }

    auto VulkanApplication::removeLayerEventHandlers(LayerName name) -> void
    {
        Application::removeLayerEventHandlers(name);
    }   
    
    VulkanApplication::~VulkanApplication()
    {
        // TODO
        shutdown();
    }

    auto keyCallback([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int32_t key, 
                            [[maybe_unused]] int32_t scancode, [[maybe_unused]] int32_t action, [[maybe_unused]] int32_t mods) -> void
    {
        auto self = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
        KeyInputEvent_data data{};
        MXC_ASSERT(sizeof(decltype(data)) <= 128, "Too big to fit inline in an Event");
         
        switch(key)
        {
	    case GLFW_KEY_UNKNOWN: data.key = KeyboardKey::UNKNOWN; break;  
	    case GLFW_KEY_SPACE: data.key = KeyboardKey::SPACE; break;  
	    case GLFW_KEY_APOSTROPHE: data.key = KeyboardKey::APOSTROPHE; break;  
	    case GLFW_KEY_COMMA: data.key = KeyboardKey::COMMA; break;  
	    case GLFW_KEY_MINUS: data.key = KeyboardKey::MINUS; break;  
	    case GLFW_KEY_PERIOD: data.key = KeyboardKey::PERIOD; break;  
	    case GLFW_KEY_SLASH: data.key = KeyboardKey::SLASH; break;  
	    case GLFW_KEY_0: data.key = KeyboardKey::_0; break;  
	    case GLFW_KEY_1: data.key = KeyboardKey::_1; break;  
	    case GLFW_KEY_2: data.key = KeyboardKey::_2; break;  
	    case GLFW_KEY_3: data.key = KeyboardKey::_3; break;  
	    case GLFW_KEY_4: data.key = KeyboardKey::_4; break;  
	    case GLFW_KEY_5: data.key = KeyboardKey::_5; break;  
	    case GLFW_KEY_6: data.key = KeyboardKey::_6; break;  
	    case GLFW_KEY_7: data.key = KeyboardKey::_7; break;  
	    case GLFW_KEY_8: data.key = KeyboardKey::_8; break;  
	    case GLFW_KEY_9: data.key = KeyboardKey::_9; break;  
	    case GLFW_KEY_SEMICOLON: data.key = KeyboardKey::SEMICOLON; break;  
	    case GLFW_KEY_EQUAL: data.key = KeyboardKey::EQUAL; break;  
	    case GLFW_KEY_A: data.key = KeyboardKey::A; break;  
	    case GLFW_KEY_B: data.key = KeyboardKey::B; break;  
	    case GLFW_KEY_C: data.key = KeyboardKey::C; break;  
	    case GLFW_KEY_D: data.key = KeyboardKey::D; break;  
	    case GLFW_KEY_E: data.key = KeyboardKey::E; break;  
	    case GLFW_KEY_F: data.key = KeyboardKey::F; break;  
	    case GLFW_KEY_G: data.key = KeyboardKey::G; break;  
	    case GLFW_KEY_H: data.key = KeyboardKey::H; break;  
	    case GLFW_KEY_I: data.key = KeyboardKey::I; break;  
	    case GLFW_KEY_J: data.key = KeyboardKey::J; break;  
	    case GLFW_KEY_K: data.key = KeyboardKey::K; break;  
	    case GLFW_KEY_L: data.key = KeyboardKey::L; break;  
	    case GLFW_KEY_M: data.key = KeyboardKey::M; break;  
	    case GLFW_KEY_N: data.key = KeyboardKey::N; break;  
	    case GLFW_KEY_O: data.key = KeyboardKey::O; break;  
	    case GLFW_KEY_P: data.key = KeyboardKey::P; break;  
	    case GLFW_KEY_Q: data.key = KeyboardKey::Q; break;  
	    case GLFW_KEY_R: data.key = KeyboardKey::R; break;  
	    case GLFW_KEY_S: data.key = KeyboardKey::S; break;  
	    case GLFW_KEY_T: data.key = KeyboardKey::T; break;  
	    case GLFW_KEY_U: data.key = KeyboardKey::U; break;  
	    case GLFW_KEY_V: data.key = KeyboardKey::V; break;  
	    case GLFW_KEY_W: data.key = KeyboardKey::W; break;  
	    case GLFW_KEY_X: data.key = KeyboardKey::X; break;  
	    case GLFW_KEY_Y: data.key = KeyboardKey::Y; break;  
	    case GLFW_KEY_Z: data.key = KeyboardKey::Z; break;  
	    case GLFW_KEY_LEFT_BRACKET: data.key = KeyboardKey::LEFT_BRACKET; break;  
	    case GLFW_KEY_BACKSLASH: data.key = KeyboardKey::BACKSLASH; break;  
	    case GLFW_KEY_RIGHT_BRACKET: data.key = KeyboardKey::RIGHT_BRACKET; break;  
	    case GLFW_KEY_GRAVE_ACCENT: data.key = KeyboardKey::GRAVE_ACCENT; break;  
	    case GLFW_KEY_WORLD_1: data.key = KeyboardKey::WORLD_1; break;  
	    case GLFW_KEY_WORLD_2: data.key = KeyboardKey::WORLD_2; break;  
	    case GLFW_KEY_ESCAPE: data.key = KeyboardKey::ESCAPE; break;  
	    case GLFW_KEY_ENTER: data.key = KeyboardKey::ENTER; break;  
	    case GLFW_KEY_TAB: data.key = KeyboardKey::TAB; break;  
	    case GLFW_KEY_BACKSPACE: data.key = KeyboardKey::BACKSPACE; break;  
	    case GLFW_KEY_INSERT: data.key = KeyboardKey::INSERT; break;  
	    case GLFW_KEY_DELETE: data.key = KeyboardKey::DELETE; break;  
	    case GLFW_KEY_RIGHT: data.key = KeyboardKey::RIGHT; break;  
	    case GLFW_KEY_LEFT: data.key = KeyboardKey::LEFT; break;  
	    case GLFW_KEY_DOWN: data.key = KeyboardKey::DOWN; break;  
	    case GLFW_KEY_UP: data.key = KeyboardKey::UP; break;  
	    case GLFW_KEY_PAGE_UP: data.key = KeyboardKey::PAGE_UP; break;  
	    case GLFW_KEY_PAGE_DOWN: data.key = KeyboardKey::PAGE_DOWN; break;  
	    case GLFW_KEY_HOME: data.key = KeyboardKey::HOME; break;  
	    case GLFW_KEY_END: data.key = KeyboardKey::END; break;  
	    case GLFW_KEY_CAPS_LOCK: data.key = KeyboardKey::CAPS_LOCK; break;  
	    case GLFW_KEY_SCROLL_LOCK: data.key = KeyboardKey::SCROLL_LOCK; break;  
	    case GLFW_KEY_NUM_LOCK: data.key = KeyboardKey::NUM_LOCK; break;  
	    case GLFW_KEY_PRINT_SCREEN: data.key = KeyboardKey::PRINT_SCREEN; break;  
	    case GLFW_KEY_PAUSE: data.key = KeyboardKey::PAUSE; break;  
	    case GLFW_KEY_F1: data.key = KeyboardKey::F1; break;  
	    case GLFW_KEY_F2: data.key = KeyboardKey::F2; break;  
	    case GLFW_KEY_F3: data.key = KeyboardKey::F3; break;  
	    case GLFW_KEY_F4: data.key = KeyboardKey::F4; break;  
	    case GLFW_KEY_F5: data.key = KeyboardKey::F5; break;  
	    case GLFW_KEY_F6: data.key = KeyboardKey::F6; break;  
	    case GLFW_KEY_F7: data.key = KeyboardKey::F7; break;  
	    case GLFW_KEY_F8: data.key = KeyboardKey::F8; break;  
	    case GLFW_KEY_F9: data.key = KeyboardKey::F9; break;  
	    case GLFW_KEY_F10: data.key = KeyboardKey::F10; break;  
	    case GLFW_KEY_F11: data.key = KeyboardKey::F11; break;  
	    case GLFW_KEY_F12: data.key = KeyboardKey::F12; break;  
	    case GLFW_KEY_F13: data.key = KeyboardKey::F13; break;  
	    case GLFW_KEY_F14: data.key = KeyboardKey::F14; break;  
	    case GLFW_KEY_F15: data.key = KeyboardKey::F15; break;  
	    case GLFW_KEY_F16: data.key = KeyboardKey::F16; break;  
	    case GLFW_KEY_F17: data.key = KeyboardKey::F17; break;  
	    case GLFW_KEY_F18: data.key = KeyboardKey::F18; break;  
	    case GLFW_KEY_F19: data.key = KeyboardKey::F19; break;  
	    case GLFW_KEY_F20: data.key = KeyboardKey::F20; break;  
	    case GLFW_KEY_F21: data.key = KeyboardKey::F21; break;  
	    case GLFW_KEY_F22: data.key = KeyboardKey::F22; break;  
	    case GLFW_KEY_F23: data.key = KeyboardKey::F23; break;  
	    case GLFW_KEY_F24: data.key = KeyboardKey::F24; break;  
	    case GLFW_KEY_F25: data.key = KeyboardKey::F25; break;  
	    case GLFW_KEY_KP_0: data.key = KeyboardKey::KP_0; break;  
	    case GLFW_KEY_KP_1: data.key = KeyboardKey::KP_1; break;  
	    case GLFW_KEY_KP_2: data.key = KeyboardKey::KP_2; break;  
	    case GLFW_KEY_KP_3: data.key = KeyboardKey::KP_3; break;  
	    case GLFW_KEY_KP_4: data.key = KeyboardKey::KP_4; break;  
	    case GLFW_KEY_KP_5: data.key = KeyboardKey::KP_5; break;  
	    case GLFW_KEY_KP_6: data.key = KeyboardKey::KP_6; break;  
	    case GLFW_KEY_KP_7: data.key = KeyboardKey::KP_7; break;  
	    case GLFW_KEY_KP_8: data.key = KeyboardKey::KP_8; break;  
	    case GLFW_KEY_KP_9: data.key = KeyboardKey::KP_9; break;  
	    case GLFW_KEY_KP_DECIMAL: data.key = KeyboardKey::KP_DECIMAL; break;  
	    case GLFW_KEY_KP_DIVIDE: data.key = KeyboardKey::KP_DIVIDE; break;  
	    case GLFW_KEY_KP_MULTIPLY: data.key = KeyboardKey::KP_MULTIPLY; break;  
	    case GLFW_KEY_KP_SUBTRACT: data.key = KeyboardKey::KP_SUBTRACT; break;  
	    case GLFW_KEY_KP_ADD: data.key = KeyboardKey::KP_ADD; break;  
	    case GLFW_KEY_KP_ENTER: data.key = KeyboardKey::KP_ENTER; break;  
	    case GLFW_KEY_KP_EQUAL: data.key = KeyboardKey::KP_EQUAL; break;  
	    case GLFW_KEY_LEFT_SHIFT: data.key = KeyboardKey::LEFT_SHIFT; break;  
	    case GLFW_KEY_LEFT_CONTROL: data.key = KeyboardKey::LEFT_CONTROL; break;  
	    case GLFW_KEY_LEFT_ALT: data.key = KeyboardKey::LEFT_ALT; break;  
	    case GLFW_KEY_LEFT_SUPER: data.key = KeyboardKey::LEFT_SUPER; break;  
	    case GLFW_KEY_RIGHT_SHIFT: data.key = KeyboardKey::RIGHT_SHIFT; break;  
	    case GLFW_KEY_RIGHT_CONTROL: data.key = KeyboardKey::RIGHT_CONTROL; break;  
	    case GLFW_KEY_RIGHT_ALT: data.key = KeyboardKey::RIGHT_ALT; break;  
	    case GLFW_KEY_RIGHT_SUPER: data.key = KeyboardKey::RIGHT_SUPER; break;  
	    case GLFW_KEY_MENU: data.key = KeyboardKey::MENU; break;  
        }
        
        EventData evData;
        memcpy(&evData, &data, sizeof(EventData));
        self->emitEvent(keyInputEvent_name, evData);
    }
    
    auto windowResized(GLFWwindow *window, int width, int height) -> void
    {
        using namespace std::literals;
	MXC_WARN("WINDOW RESIZED EVENT %dx%d", width, height);
        auto self = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
	//auto* ctx = self->m_renderer.getContextPointer();
	//self->m_renderer.resizeFramebufferSizes(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        //self->m_renderer.onResize();
	EventData resizeEventData; 
	resizeEventData.data.u32[0] = static_cast<uint32_t>(width);
	resizeEventData.data.u32[1] = static_cast<uint32_t>(height);
	resizeEventData.data.u32[2] = 0;
	resizeEventData.data.u32[3] = 0;
	self->emitEvent(windowResizedEvent_name, resizeEventData);
    }

}
