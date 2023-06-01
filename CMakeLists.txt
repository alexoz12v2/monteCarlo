cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
set(CMAKE_VERBOSE_MAKEFILE ON)
# inputs to this cmake file:
#   VULKAN_SDK              path to the vulkan sdk root, where the setup_env.sh file is located. Optional if VULKAN_SDK enviroment variables are 
#                           already set by the user, eg in ~/.zshrc
# macros you can define:
#   NDEBUG                  standard C++ macro which i'll use to define whether to compile with debug symbols or not    

project(Project)

# using cmake module FindVulkan
include(FindVulkan)

# taken from Learning Vulkan Chapter3
# AUTO_LOCATE_VULKAN - accepted value ON or OFF
# ON  - Use CMake to auto locate the Vulkan SDK.
# OFF - Vulkan SDK path can be specified manually. This is helpful to test the build on various Vulkan version.
option(AUTO_LOCATE_VULKAN "AUTO_LOCATE_VULKAN" ON) # equivalent to set(AUTO_LOCATE_VULKAN true CACHE BOOL "AUTO_LOCATE_VULKAN")

# TODO clean up macro defs
# these are also added in the src/CMakeLists 
############### Code Debugging Macros #########################################
add_compile_definitions(
    MXC_LOGLEVEL_INFO
    MXC_LOGLEVEL_WARN
    MXC_LOGLEVEL_ERROR
    MXC_LOGLEVEL_TRACE
    MXC_LOGLEVEL_DEBUG
    _DEBUG
)

############# Locate Vulkan ###################################################
if(DEFINED ENV{VULKAN_SDK})
    set(VulkanSDK_FOUND ON CACHE BOOL "VulkanSDK_FOUND")
    set(VulkanSDK_INCLUDE_DIRS "$ENV{VULKAN_SDK}/include")
    set(VulkanSDK_LIBRARIES "$ENV{VULKAN_SDK}/lib")
    message(STATUS "VulkanSDK found at $ENV{VULKAN_SDK}")
endif()

if(AUTO_LOCATE_VULKAN)
    message(STATUS "Attempting to locate Vulkan ...")

    # specialized version of find_package, link https://cmake.org/cmake/help/latest/module/FindVulkan.html
    # if you provide no arguments to it, it is equivalent to find_package(Vulkan::Vulkan). this module will define, among others
    # Vulkan_FOUND              boolean, true if Vulkan was found
    # Vulkan_INCLUDE_DIRS       include directories for Vulkan
    # Vulkan_LIBRARIES          link against thsi library to use Vulkan
    # you can specify more optional components to search for, e.g. Vulkan::dxc_lib, which will in turn define
    # Vulkan_dxc_lib_FOUND and Vulkan_dxc_LIBRARY
    find_package(Vulkan REQUIRED) # if doesn't work put find_package(Vulkan REQUIRED)
endif()

if(NOT Vulkan_FOUND)
    # give up your spirits
    message(FATAL_ERROR "Could not find Vulkan library!")
    return()
endif()

string(REGEX REPLACE "/include" "" VULKAN_PATH "${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan Library found at ${VULKAN_PATH} include ${Vulkan_INCLUDE_DIRS}")

################## Define Platform Macros #####################################
if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
elseif(LINUX)
    if(USE_WAYLAND) # doesn't work probably
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WAYLAND_KHR")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XCB_KHR")
        find_package(X11 REQUIRED)
    endif()
elseif(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_MACOS_MVK")
endif()

##################### Find any other external dependencies ####################
# TODO could be refactored in a function
################# GLFW ################
find_package(glfw3 3.3 REQUIRED)
if(NOT glfw3_FOUND AND UNIX)
    # trying again
    set(CMAKE_PREFIX_PATH "/usr/lib/cmake/glfw3")
    find_package(glfw3 3.3 REQUIRED)
endif()
if(NOT glfw3_FOUND)
    # fallback to git submodule
    message(WARNING "package glfw couldn't be found, fallback to external/glfw") 

    # disable build of docs, tests, examples
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    add_subdirectory("${PROJECT_SOURCE_DIR}/external/glfw")
    set(glfw3_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/glfw/include")
else()
    message(STATUS "installed package glfw3 found at ${glfw3_DIR}")
endif()

# ############## Eigen ################
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
if(NOT Eigen3_FOUND)
    # fallback to git submodule
    message(WARNING "package Eigen3 couldn't be found, fallback to external/Eigen") 

    add_subdirectory("${PROJECT_SOURCE_DIR}/external/Eigen3")
    set(Eigen3_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/Eigen3")
else()
    message(STATUS "installed package Eigen3 found at ${Eigen3_DIR}")
endif()

# ######### tinyobj loader ############
find_package(tinyobjloader)
if(NOT tinyobjloader_FOUND)
    # fallback to git submodule
    message(WARNING "package tinyobjloader couldn't be found, fallback to external/glfw") 

    set(tinyobjloader_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/tinyobjloader")
else()
    message(STATUS "installed package tinyobjloader found at ${tinyobjloader_DIR}")
endif()

# ############## fmt ##################
find_package(fmt)
if(NOT fmt_FOUND)
    # fallback to git submodule
    message(WARNING "package fmt couldn't be found, fallback to external/glfw") 

    add_subdirectory("${PROJECT_SOURCE_DIR}/external/fmt")
    set(fmt_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/fmt/include")
else()
    message(STATUS "installed package fmt found at ${fmt_DIR}")
endif()

# ##### Vulkan Memory Allocator #######
add_subdirectory("${PROJECT_SOURCE_DIR}/external/VulkanMemoryAllocator")
set(VulkanMemoryAllocator_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/VulkanMemoryAllocator/include")

# ############ refl ###################
set(refl_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/refl")

# directories
if(DEFINED ENV{VULKAN_SDK})
    # target_include_directories(Project PUBLIC
    include_directories(PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${Vulkan_INCLUDE_DIRS}
        $ENV{VulkanSDK_INCLUDE_DIRS}
        ${glfw3_INCLUDE_DIRS}
        ${Eigen3_INCLUDE_DIRS}
        ${tinyobjloader_INCLUDE_DIRS}
        ${fmt_INCLUDE_DIRS}
        ${VulkanMemoryAllocator_INCLUDE_DIRS}
        ${refl_INCLUDE_DIRS}
        )
else()
    # target_include_directories(Project PUBLIC
    include_directories(PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${Vulkan_INCLUDE_DIRS}
        ${glfw3_INCLUDE_DIRS}
        ${Eigen3_INCLUDE_DIRS}
        ${tinyobjloader_INCLUDE_DIRS}
        ${fmt_INCLUDE_DIRS}
        ${VulkanMemoryAllocator_INCLUDE_DIRS}
        ${refl_INCLUDE_DIRS}
        )
endif()

# my own source
add_subdirectory("${PROJECT_SOURCE_DIR}/src")

# link
#TODO fix issue with dxc. Also remove refl
set(EXEC_LIBS "${Vulkan_LIBRARIES}" glfw Eigen3::Eigen fmt::fmt "/run/media/alessio/5b5d3976-c146-4dae-b622-58b92b81d64f/HDD/DownloadsHDD/vulkansdk-linux-x86_64-1.3.243.0/1.3.243.0/x86_64/lib/libdxcompiler.so" Renderer) 

if(LINUX)
    if(USE_WAYLAND)
        set(EXEC_LIBS ${EXEC_LIBS} wayland-client wayland-egl)
    else()
        set(EXEC_LIBS ${EXEC_LIBS} X11-xcb xcb) 
    endif(USE_WAYLAND)
elseif(APPLE)
    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -framework Cocoa")
elseif(WIN32)
    set(EXEC_LIBS ${EXEC_LIBS} User32 Gdi32)
endif(LINUX)

#target_link_directories(Project PUBLIC "$ENV{VulkanSDK_LIBRARIES}")

# Define project properties (From Learning Vulkan by packt)
set_property(TARGET ${Recipe_Name} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)
set_property(TARGET ${Recipe_Name} PROPERTY RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/build)
set_property(TARGET ${Recipe_Name} PROPERTY RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/build)
set_property(TARGET ${Recipe_Name} PROPERTY RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_CURRENT_SOURCE_DIR}/build)
set_property(TARGET ${Recipe_Name} PROPERTY RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_CURRENT_SOURCE_DIR}/build)

# build executables
set(INCLUDE_HEADERS 
    "${CMAKE_CURRENT_SOURCE_DIR}/Device.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Renderer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Swapchain.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/CommandBuffer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/logging.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shader.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Pipeline.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Application.h"
    )

set(SHADER_PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/execSrc)
