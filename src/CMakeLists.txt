add_library(Renderer Renderer.h Renderer.cpp)

# generate position independent code for this library, which allows the linker to merge the output with 
# the main executable output. 
# current compilers default to position independent for everything due to security advantages
# set_target_properties(Renderer PROPERTIES POSITION_INDEPENDENT_CODE ON)

####################### Compiler Options ######################################
option(USE_WAYLAND "" OFF)

if (DEFINED _DEBUG)
    message(STATUS "debug mode ON")
endif()

if(WIN32)
    # TODO C++ compilation with \Wx
    if(NOT DEFINED _DEBUG)
        target_compile_options(Renderer PRIVATE /std:c++20 /O2 /W4 )
    else()
        target_compile_options(Renderer PRIVATE /std:c++20 /O0 /W4 /DEBUG)
    endif()
else()
    # Set the C and C++ compilers to Clang
    set(CMAKE_C_COMPILER "/usr/bin/clang")
    set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
    if(NOT DEFINED _DEBUG)
        target_compile_options(Renderer PRIVATE -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror
            # VMA generates these warnings
            -Wno-nullability-extension -Wno-nullability-completeness
            # used by dxc
            -Wno-language-extension-token -fms-extensions -Wno-ambiguous-reversed-operator
            # TODO logging generates these warnings. Fix it
            -Wno-gnu-zero-variadic-macro-arguments
            -Wno-switch)
    else()
        target_compile_options(Renderer PRIVATE -std=c++20 -O0 -Wall -Wextra -Wpedantic -g -Werror
            # VMA generates these warnings
            -Wno-nullability-extension -Wno-nullability-completeness
            # TODO logging generates these warnings. Fix it
            -Wno-gnu-zero-variadic-macro-arguments
            # used by dxc
            -Wno-language-extension-token -fms-extensions -Wno-ambiguous-reversed-operator
            # this is useless when working with Vulkan
            -Wno-switch)
    endif()
endif()

#################### sources ##################################################
target_include_directories(Renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(Renderer PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/Device.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Renderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Swapchain.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CommandBuffer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/logging.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Pipeline.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Application.cpp"
    )
