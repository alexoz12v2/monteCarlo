#include "logging.h" // LOGGING GOES BEFORE RENDERER AND OTHER STUFF

#include "Renderer.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#include <fmt/core.h>
#include <X11/Xlib-xcb.h>

// shader stuff
#include <dxc/dxcapi.h>

#include <cstdint>
#include <cstdlib>
#include <string>
#include <array>
#include <memory>

#define CHECK_DXC_RESULT(idxcblob) \
    do{HRESULT status;\
    idxcblob->GetStatus(&status);\
    assert(status == S_OK);}while(0)

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

// https://registry.khronos.org/vulkan/site/guide/latest/hlsl.html
auto compileShader(std::wstring filename) -> CComPtr<IDxcBlob> // TODO remove string
{
    // TODO RELEASE ALL MEMORY
    static bool compilerUninitialized = true;
    static CComPtr<IDxcLibrary> pLibrary{nullptr};
    static CComPtr<IDxcUtils> pUtils{nullptr};
    static CComPtr<IDxcCompiler3> pCompiler{nullptr};

    HRESULT hres;

    if (compilerUninitialized) [[unlikely]]
    {
        compilerUninitialized = false;

        // Initialize DXC library
        hres = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&pLibrary));
        assert(!FAILED(hres));

        // initialize DXC compiler
        hres = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler));
        assert(!FAILED(hres));

        // initialize DXC utility
        hres = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
        assert(!FAILED(hres));
    }

    // Load HLSL shader from disk
    uint32_t codePage = DXC_CP_ACP; // ansi code page i.e. the default for the system
    CComPtr<IDxcBlobEncoding> pSourceBlob{nullptr};
    hres = pUtils->LoadFile(filename.c_str(), &codePage, &pSourceBlob);
    assert(!FAILED(hres));

    // select shader target profile based on extension (basically the version of the shader language, model, type of shader)
    LPCWSTR targetProfile{};
    size_t idx = filename.rfind('.');
    if (idx != std::string::npos)
    {
        std::wstring extension = filename.substr(idx+1);
        if (extension == L"vert")
            targetProfile = L"vs_6_1";
        else if (extension == L"frag")
            targetProfile = L"ps_6_1";
        else if (extension == L"comp")
            targetProfile = L"cs_6_1";
        // TODO anything else
    }

    // compile shader
    std::array args { (LPCWSTR)
        filename.c_str(),               // optional filename to be displayed in case of compilation error
        L"-Zpc",                        // matrices in column-major order
        L"-HV", L"2021",                // HLSL version 2021
        L"-T", targetProfile,           // targetProfile       
        L"-E", L"main",                 // entryPoint TODO might expose as parameter
        L"-spirv",                      // compile to spirv
        L"-fspv-target-env=vulkan1.3"   // use vulkan1.3 environment
    };

    DxcBuffer srcBuffer {
        .Ptr = pSourceBlob->GetBufferPointer(),
        .Size = pSourceBlob->GetBufferSize(),
        .Encoding = DXC_CP_ACP // 
    };

    CComPtr<IDxcResult> pResult{nullptr};
    hres = pCompiler->Compile(
        &srcBuffer,         // DxcBuffer containing the shader source
        args.data(),        // arguments passed to the dxc compiler
        static_cast<uint32_t>(args.size()),
        nullptr,            // optional include handler to manage #includes in the shaders TODO 
        IID_PPV_ARGS(&pResult)
    );

    // Compilation failure handling
    if (FAILED(hres) && (pResult)) // checking result because the compile function might fail because of a compilation error or memory exhaustion
    {
        CComPtr<IDxcBlobEncoding> pErrorBlob;
        hres = pResult->GetErrorBuffer(&pErrorBlob);
        if (SUCCEEDED(hres) && pErrorBlob) [[likely]]
        {
            fmt::print("\033[31m[ERROR] Vertex Shader Compilation Error: {}\033[0m\n", reinterpret_cast<char*>(&pErrorBlob));
            assert(false);
        }
    }

    // get the compilation result
    CComPtr<IDxcBlob> spirvBinaryCode;
    pResult->GetResult(&spirvBinaryCode);
    return spirvBinaryCode;
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
    CComPtr<IDxcBlob> shaderObjVs = compileShader(L"../shaders/cubes.vert");
    CComPtr<IDxcBlob> shaderObjFs = compileShader(L"../shaders/cubes.frag");

    mxc::Renderer renderer;
    mxc::RendererConfig config {};
    config.platformSurfaceExtensionName = "VK_KHR_xcb_surface";
    config.windowWidth = width;
    config.windowHeight = height;
    config.connection = XGetXCBConnection(glfwGetX11Display());
    config.window = glfwGetX11Window(window);

    assert(renderer.init(config));
    assert(false);

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
