#if !defined(MXC_VULKAN_APPLICATION_H)
#define MXC_VULKAN_APPLICATION_H

#define MXC_PREVENT_APPLICATION_DEFINE_MAIN
#include "KeyboardKey.h"
#include "Application.h"
#include "Renderer.h"

#include "GLFW/glfw3.h"

namespace mxc
{
	struct KeyInputEvent_data
	{
		KeyboardKey key;
	};

	struct WindowExtent {uint32_t width, height;};

	static EventName constexpr keyInputEvent_name = "keyInputEvent";
	static EventName constexpr windowResizedEvent_name = "windowResizedEvent";

	class VulkanApplication : public Application
	{
		friend auto ::main([[maybe_unused]]int32_t argc, [[maybe_unused]]char** argv) -> int32_t; 
		friend auto windowResized(GLFWwindow *window, int width, int height) -> void;

	public:
		VulkanApplication(VulkanApplication const&) = delete;
		VulkanApplication(VulkanApplication&&) = delete;
		auto operator=(VulkanApplication const&) -> VulkanApplication& = delete;
		auto operator=(VulkanApplication&&) -> VulkanApplication& = delete;

	public:
		~VulkanApplication();

	public:
		auto getRenderer() & -> Renderer& { return m_renderer; }
		auto getWindowExtent() const -> WindowExtent
		{ 
			int32_t width, height;
			glfwGetFramebufferSize(m_window, &width, &height);
			return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
		}

	private:
		auto run() -> void;
		
		auto init() -> bool;
		auto tick(float deltaTime) -> void;
		auto shutdown() -> void;

		auto handleSignal(ApplicationSignal_t sig, uint32_t emitterLayerIndex) -> bool;
		auto removeLayerEventHandlers(LayerName name) -> void;
	
	protected:
		VulkanApplication() : Application(), m_window(nullptr) {}

	private:
		GLFWwindow* m_window;
		Renderer m_renderer;
	};

}

#if !defined(MXC_PREVENT_VULKAN_APPLICATION_DEFINE_MAIN)
#define MXC_PREVENT_VULKAN_APPLICATION_DEFINE_MAIN

auto initializeApplication(mxc::VulkanApplication& app, int32_t argc, char** argv) -> bool;

auto main([[maybe_unused]]int32_t argc, [[maybe_unused]]char** argv) -> int32_t
{
    mxc::VulkanApplication app;

    MXC_INFO("Initializing application...");
    if (!initializeApplication(app, argc, argv) || !app.init())
    {	
        MXC_ERROR("Couldn't initialize application");
        return 1;
    }

    MXC_INFO("Running the application...");
    app.run();
	MXC_ERROR("--- OUT OF SCOPE ---");
}

#endif //MXC_PREVENT_VULKAN_APPLICATION_DEFINE_MAIN

#endif // MXC_VULKAN_APPLICATION_H
