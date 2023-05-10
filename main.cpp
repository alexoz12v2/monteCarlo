#include <GLFW/glfw3.h>
#include <fmt/core.h>

// include bx BEFORE bgfx, otherwise name clashing
#define BX_CONFIG_DEBUG 1
#include <bx/bx.h>
#include <bx/timer.h>
#include <bx/math.h>

#include <bgfx/bgfx.h>

#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>

// shader stuff
#include <dxc/dxcapi.h>

#include <cstdint>
#include <cstdlib>
#include <string_view>
#include <array>
#include <memory>

#define BGFX_CHECK_VALID(handle) \
    if (!bgfx::isValid(handle)) { fmt::print("Failed to create {}\n", #handle); };

#define CHECK_DXC_RESULT(idxcblob) \
    do{HRESULT status;\
    idxcblob->GetStatus(&status);\
    assert(status == S_OK);}while(0);

// using raw strings for simplicity
std::string_view s_vsShaderSource = R"(
    struct VS_INPUT
    {
        float3 pos : POSITION;
        uint color : COLOR;
    };

    struct VS_OUTPUT
    {
        float4 pos : SV_POSITION;
        uint color : COLOR;
    };

    VS_OUTPUT main(VS_INPUT input)
    {
        VS_OUTPUT output;
        output.pos = float4(input.pos, 1.0);
        output.color = input.color;
        return output;
    }
)";
std::string_view s_fsShaderSource = R"(
    struct FSInput {
        uint color : COLOR0;
    };

    float4 main(FSInput input) : SV_TARGET {
        float4 color = float4(
            ((input.color >> 16) & 0xff) / 255.0f,
            ((input.color >> 8) & 0xff) / 255.0f,
            (input.color & 0xff) / 255.0f,
            ((input.color >> 24) & 0xff) / 255.0f
        );
        return color;
    }
)";

// stolen cube data
struct PosColorVertex
{
    float x;
    float y;
    float z;
    uint32_t abgr;
};

static PosColorVertex s_cubeVertices[] =
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

static const uint16_t s_cubeTriList[] =
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
    GLFWwindow* window = nullptr;
    if (!createAppWindow(&window))
    {
        return 1;
    }

    glfwSetKeyCallback(window, keyCallback);

    int32_t width, height;
    glfwGetFramebufferSize(window, &width, &height);

    // initialize the bgfx library
    bgfx::Init bgfxInit;
    bgfxInit.type = bgfx::RendererType::Vulkan;
    bgfxInit.platformData.nwh = reinterpret_cast<void*>(glfwGetX11Window(window));
    bgfxInit.platformData.ndt = reinterpret_cast<void*>(glfwGetX11Display());
    bgfxInit.resolution.width = width;
    bgfxInit.resolution.height = height;
    bgfxInit.resolution.reset = BGFX_RESET_VSYNC;

    bgfx::init(bgfxInit);

    // set view clear flags and rect 
    bgfx::setViewClear(/*view id?*/0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, /*color*/0x44'33'55'FF, /*depth*/1.f, /*stencil*/0.f); // sets clear values

    // create a vertex buffer (static data has to be passed with bgfc::makeRef)
    bgfx::VertexLayout vLayout;
    vLayout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, /*normalized*/true)
        .end();
    bgfx::VertexBufferHandle vBuffer = bgfx::createVertexBuffer(bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices)), vLayout);

    // create an index buffer with triangle list topology
    bgfx::IndexBufferHandle iBuffer = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeTriList, sizeof(s_cubeTriList)));

    // load program from shaders step1: compile HLSL shaders to SPIR-V
    // Compile vertex shader
#if 0
    IDxcLibrary* dxcLibrary;
    IDxcCompiler* dxcCompiler;
    DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&dxcLibrary));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));

    IDxcBlobEncoding* vsSource;
    dxcLibrary->CreateBlobWithEncodingFromPinned(s_vsShaderSource, sizeof(s_vsShaderSource ), CP_UTF8, &vsSource);

    IDxcOperationResult* vsResult;
    dxcCompiler->Compile(vsSource, L"simpleVertexShader.hlsl", L"main", L"vs_6_0", nullptr, 0, nullptr, 0, nullptr, &vsResult);
    IDxcBlob* vsBlob;
    vsResult->GetResult(&vsBlob);

    // Compile fragment shader
    IDxcBlobEncoding* fsSource;
    dxcLibrary->CreateBlobWithEncodingFromPinned(s_fsShaderSource, sizeof(s_fsShaderSource), CP_UTF8, &fsSource);

    IDxcOperationResult* fsResult;
    dxcCompiler->Compile(fsSource, L"simpleFragmentShader.hlsl", L"main", L"ps_6_0", nullptr, 0, nullptr, 0, nullptr, &fsResult);
    IDxcBlob* fsBlob;
    fsResult->GetResult(&fsBlob);

    bgfx::Memory const* vsShaderSpirV = bgfx::makeRef(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize());
    bgfx::Memory const* fsShaderSpirV = bgfx::makeRef(fsBlob->GetBufferPointer(), fsBlob->GetBufferSize());

    // load program from shaders step2: create ShaderHandles
    bgfx::ShaderHandle vsShaderHandle = bgfx::createShader(vsShaderSpirV);
    BGFX_CHECK_VALID(vsShaderHandle);
    bgfx::ShaderHandle fsShaderHandle = bgfx::createShader(fsShaderSpirV);
    BGFX_CHECK_VALID(fsShaderHandle);

    // load program from shaders step2: create ShaderHandles
    bgfx::ProgramHandle program = bgfx::createProgram(vsShaderHandle, fsShaderHandle, true/*destroy shaders when program is destroyed*/);
    BGFX_CHECK_VALID(program);
#else // TODO RELEASE ALL MEMORY
    CComPtr<IDxcUtils> dxcUtils = {};
    CComPtr<IDxcCompiler3> dxcCompiler = {};
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));

    DxcBuffer srcBufferVs = {
        .Ptr = &*s_vsShaderSource.begin(),
        .Size = static_cast<uint32_t>(s_vsShaderSource.size()),
        .Encoding = 0 // ascii
    };
    CComPtr<IDxcResult> result;
    // matrices column major | HLSL version 2021 | shadertype and model | entry point = main | spirv | vulkan 1.3
    std::array argsVs { (LPCWSTR)L"-Zpc",L"-HV",L"2021",L"-T",L"vs_6_0",L"-E",L"main",L"-spirv",L"-fspv-target-env=vulkan1.3"};
    dxcCompiler->Compile(&srcBufferVs, argsVs.data(), static_cast<uint32_t>(argsVs.size()), nullptr, IID_PPV_ARGS(&result));
    CHECK_DXC_RESULT(result);

    // exrtact shader obj
    CComPtr<IDxcBlob> shaderObjVs;
    result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderObjVs), nullptr);
    bgfx::Memory const* vsShaderSpirV = bgfx::alloc(shaderObjVs->GetBufferSize());
    memcpy(vsShaderSpirV->data, shaderObjVs->GetBufferPointer(), shaderObjVs->GetBufferSize());

    bgfx::ShaderHandle vsShaderHandle = bgfx::createShader(vsShaderSpirV);
    BGFX_CHECK_VALID(vsShaderHandle);

    result.Release();
    DxcBuffer srcBufferFs {
        .Ptr = &*s_fsShaderSource.begin(),
        .Size = static_cast<uint32_t>(s_fsShaderSource.size()),
        .Encoding = 0
    };
    std::array argsFs {(LPCWSTR)L"-Zpc",L"-HV",L"2021",L"-T",L"ps_6_0",L"-E",L"main",L"-spirv",L"-fspv-target-env=vulkan1.3"};
    dxcCompiler->Compile(&srcBufferFs, argsFs.data(), static_cast<uint32_t>(argsFs.size()), nullptr, IID_PPV_ARGS(&result));
    CHECK_DXC_RESULT(result);

    CComPtr<IDxcBlob> shaderObjFs;
    result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderObjFs), nullptr);
    bgfx::Memory const* fsShaderSpirV = bgfx::alloc(shaderObjFs->GetBufferSize());
    memcpy(fsShaderSpirV->data, shaderObjFs->GetBufferPointer(), shaderObjFs->GetBufferSize());

    bgfx::ShaderHandle fsShaderHandle = bgfx::createShader(fsShaderSpirV);
    BGFX_CHECK_VALID(fsShaderHandle);

    bgfx::ProgramHandle program = bgfx::createProgram(vsShaderHandle, fsShaderHandle, true);
    BGFX_CHECK_VALID(program);
#endif
    // main loop
    float time = bx::getHPCounter();
    while (!glfwWindowShouldClose(window))
    {
        time = (float)( (bx::getHPCounter() - time)/double(bx::getHPFrequency() ) );
        // render frame
        {
            // sets 0 as the default viewport
            const bx::Vec3 at  = { 0.0f, 0.0f,   0.0f };
            const bx::Vec3 eye = { 0.0f, 0.0f, -35.0f };
            // Set view and projection matrix for view 0.
	        {
                float view[16];
                bx::mtxLookAt(view, eye, at);

                float proj[16];
                bx::mtxProj(proj, 60.0f, float(width)/float(height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
                bgfx::setViewTransform(0, view, proj);

                // Set view 0 default viewport.
                bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height) );
	        }           
            // make sure trhat no other draw calls are submitted to view 0
            bgfx::touch(0);

            // Submit 11x11 cubes.
            for (uint32_t yy = 0; yy < 11; ++yy)
            {
                for (uint32_t xx = 0; xx < 11; ++xx)
                {
                    float mtx[16];
                    bx::mtxRotateXY(mtx, time + xx*0.21f, time + yy*0.37f);
                    mtx[12] = -15.0f + float(xx)*3.0f;
                    mtx[13] = -15.0f + float(yy)*3.0f;
                    mtx[14] = 0.0f;

                    // Set model matrix for rendering.
                    bgfx::setTransform(mtx);

                    // Set vertex and index buffer.
                    bgfx::setVertexBuffer(0, vBuffer);
                    bgfx::setIndexBuffer(iBuffer);

                    // Set render states.
                    uint64_t state = BGFX_STATE_WRITE_R | BGFX_STATE_WRITE_G | BGFX_STATE_WRITE_B | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z
                        | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW;
                    bgfx::setState(state);

                    // Submit primitive for rendering to view 0.
                    bgfx::submit(0, program);
                }
            }

	        // Advance to next frame. Rendering thread will be kicked to
	        // process submitted rendering primitives.
	        bgfx::frame();

        }

        // lock to 60 fps
        glfwWaitEventsTimeout(0.016666666666f);
    }

    // clean up bgfx
    if (bgfx::isValid(program) && bgfx::isValid(iBuffer) && bgfx::isValid(vBuffer))
    {
        bgfx::destroy(iBuffer);
        bgfx::destroy(vBuffer);
        bgfx::destroy(program); // crashes
        bgfx::shutdown();
    }
    else
    {
        if (!bgfx::isValid(program)) fmt::print("NOT VALID: program\n");
        if (!bgfx::isValid(iBuffer)) fmt::print("NOT VALID: iBuffer\n");
        if (!bgfx::isValid(vBuffer)) fmt::print("NOT VALID: vBuffer\n");
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}

static auto keyCallback([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int32_t key, [[maybe_unused]] int32_t scancode, [[maybe_unused]] int32_t action, [[maybe_unused]] int32_t mods) -> void
{
    // fmt::print("{} key\n", glfwGetKeyName(key, 0));
}
