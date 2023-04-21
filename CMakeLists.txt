cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

# inputs to this cmake file:
#   VULKAN_SDK              path to the vulkan sdk root, where the setup_env.sh file is located. Optional if VULKAN_SDK enviroment variables are 
#                           already set by the user, eg in ~/.zshrc
# macros you can define:
#   NDEBUG                  standard C++ macro which i'll use to define whether to compile with debug symbols or not    

project("MonteCarlo")
add_executable(Project main.cpp)

# using cmake module FindVulkan
include(FindVulkan)

# taken from Learning Vulkan Chapter3
# AUTO_LOCATE_VULKAN - accepted value ON or OFF
# ON  - Use CMake to auto locate the Vulkan SDK.
# OFF - Vulkan SDK path can be specified manually. This is helpful to test the build on various Vulkan version.
option(AUTO_LOCATE_VULKAN "AUTO_LOCATE_VULKAN" ON) # equivalent to set(AUTO_LOCATE_VULKAN true CACHE BOOL "AUTO_LOCATE_VULKAN")

############# Locate Vulkan ###################################################
if(DEFINED ENV{VULKAN_SDK})
    set(VulkanSDK_FOUND ON CACHE BOOL "VulkanSDK_FOUND")
    set(VulkanSDK_INCLUDE_DIRS "$ENV{VULKAN_SDK}/include")
    set(VulkanSDK_LIBRARIES "$ENV{VULKAN_SDK}/lib")
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

# directories
if(DEFINED ENV{VULKAN_SDK})
    target_include_directories(Project PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${Vulkan_INCLUDE_DIRS}
        ${VulkanSDK_INCLUDE_DIRS}
        ${glfw3_INCLUDE_DIRS}
        ${Eigen3_INCLUDE_DIRS}
        ${tinyobjloader_INCLUDE_DIRS}
        ${fmt_INCLUDE_DIRS}
        ${VulkanMemoryAllocator_INCLUDE_DIRS }
        )
else()
    target_include_directories(Project PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${Vulkan_INCLUDE_DIRS}
        ${glfw3_INCLUDE_DIRS}
        ${Eigen3_INCLUDE_DIRS}
        ${tinyobjloader_INCLUDE_DIRS}
        ${fmt_INCLUDE_DIRS}
        ${VulkanMemoryAllocator_INCLUDE_DIRS }
        )
endif()
# link
if(DEFINED ENV{VULKAN_SDK})
    target_link_libraries(Project PUBLIC "${Vulkan_LIBRARIES}" glfw Eigen3::Eigen fmt::fmt)
    target_link_directories(Project PUBLIC "${VulkanSDK_LIBRARIES}")
else()
    target_link_libraries(Project PUBLIC "${Vulkan_LIBRARIES}" glfw Eigen3::Eigen fmt::fmt)
endif()

####################### Compiler Options ######################################
if(WIN32)
    if(DEFINED NDEBUG)
        target_compile_options(Project PRIVATE /std:c++20 /O2 /W4 /WX)
    else()
        target_compile_options(Project PRIVATE /std:c++20 /O0 /W4 /WX /DEBUG)
    endif()
else()
    set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
    if(DEFINED NDEBUG)
        target_compile_options(Project PRIVATE -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror)
    else()
        target_compile_options(Project PRIVATE -std=c++20 -O0 -Wall -Wextra -Wpedantic -Werror -g)
    endif()
endif()

# Define project properties (From Learning Vulkan by packt)
set_property(TARGET ${Recipe_Name} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)
set_property(TARGET ${Recipe_Name} PROPERTY RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/build)
set_property(TARGET ${Recipe_Name} PROPERTY RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/build)
set_property(TARGET ${Recipe_Name} PROPERTY RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_CURRENT_SOURCE_DIR}/build)
set_property(TARGET ${Recipe_Name} PROPERTY RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_CURRENT_SOURCE_DIR}/build)
