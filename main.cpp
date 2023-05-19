#include "logging.h" // LOGGING GOES BEFORE RENDERER AND OTHER STUFF

#include "Renderer.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#include <fmt/core.h>
#include <X11/Xlib-xcb.h>

#include <cstdint>
#include <cstdlib>
#include <string>
#include <array>
#include <memory>

// stolen cube data
struct PosColorVertex
{
    float x;
    float y;
    float z;
    uint32_t abgr;
};

[[maybe_unused]] static PosColorVertex s_cubeVertices[] =
{
    {-1.0f,  1.0f,  1.0f, 0xff000000 },
    { 1.0f,  1.0f,  1.0f, 0xff0000ff },
    {-1.0f, -1.0f,  1.0f, 0xff00ff00 },
    { 1.0f, -1.0f,  1.0f, 0xff00ffff },
    {-1.0f,  1.0f, -1.0f, 0xffff0000 },
    { 1.0f,  1.0f, -1.0f, 0xffff00ff },
    {-1.0f, -1.0f, -1.0f, 0xffffff00 },
    { 1.0f, -1.0f, -1.0f, 0xffffffff },
};

[[maybe_unused]] static const uint16_t s_cubeTriList[] =
{
    0, 1, 2,
    1, 3, 2,
    4, 6, 5,
    5, 6, 7,
    0, 2, 4,
    4, 2, 6,
    1, 5, 3,
    5, 7, 3,
    0, 4, 1,
    4, 5, 1,
    2, 3, 6,
    6, 3, 7,
};

static auto keyCallback(GLFWwindow* window, int32_t key, int32_t scancode, int32_t action, int32_t mods) -> void;

auto createAppWindow(GLFWwindow** outWindow) -> bool
{
    static int32_t constexpr startWindowWidth = 640, startWindowHeight = 480;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    *outWindow = glfwCreateWindow(startWindowWidth, startWindowHeight, "Test App", nullptr, nullptr);
    if (!*outWindow)
    {
        glfwTerminate();
        return false;
    }

    return true;
}


auto main() -> int32_t
{
    // initialize GLFW library
    if (!glfwInit())
    {
        return 1;
    }

    // GLFWwindow creation
    MXC_INFO("creating the window");
    GLFWwindow* window = nullptr;
    if (!createAppWindow(&window))
    {
        return 1;
    }

    glfwSetKeyCallback(window, keyCallback);

    int32_t width, height;
    glfwGetFramebufferSize(window, &width, &height);

    // load program from shaders step1: compile HLSL shaders to SPIR-V
    // CComPtr<IDxcBlob> shaderObjVs = compileShader(L"../shaders/cubes.vert");
    // CComPtr<IDxcBlob> shaderObjFs = compileShader(L"../shaders/cubes.frag");

    mxc::Renderer renderer;
    mxc::RendererConfig config {};
    config.platformSurfaceExtensionName = "VK_KHR_xcb_surface";
    config.windowWidth = width;
    config.windowHeight = height;
    config.connection = XGetXCBConnection(glfwGetX11Display());
    config.window = glfwGetX11Window(window);

    assert(renderer.init(config));

    // main loop
    while (!glfwWindowShouldClose(window))
    {
        // render frame

        // lock to 60 fps
        glfwWaitEventsTimeout(0.016666666666f);
    }

    renderer.cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();
}

static auto keyCallback([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int32_t key, [[maybe_unused]] int32_t scancode, [[maybe_unused]] int32_t action, [[maybe_unused]] int32_t mods) -> void
{
    // fmt::print("{} key\n", glfwGetKeyName(key, 0));
}
