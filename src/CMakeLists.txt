add_library(Renderer Renderer.h Renderer.cpp)

# generate position independent code for this library, which allows the linker to merge the output with 
# the main executable output. 
set_target_properties(Renderer PROPERTIES POSITION_INDEPENDENT_CODE ON)

####################### Compiler Options ######################################
option(USE_WAYLAND "" OFF)

if(WIN32)
    if(DEFINED NDEBUG)
        target_compile_options(Renderer PRIVATE /std:c++20 /O2 /W4 )
    else()
        target_compile_options(Renderer PRIVATE /std:c++20 /O0 /W4 /DEBUG)
    endif()
else()
    # Set the C and C++ compilers to Clang
    set(CMAKE_C_COMPILER "/usr/bin/clang")
    set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
    if(DEFINED NDEBUG)
        target_compile_options(Renderer PRIVATE -std=c++20 -O2 -Wall -Wextra -Wpedantic)
    else()
        target_compile_options(Renderer PRIVATE -std=c++20 -O0 -Wall -Wextra -Wpedantic -g)
    endif()
endif()

#################### sources ##################################################
target_include_directories(Renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(Renderer PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/Device.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Renderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Swapchain.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/logging.cpp"
    )

