cmake_minimum_required(VERSION 3.25)
project(MyLearnWebGPU C CXX)
set(CMAKE_CXX_STANDARD 20)

include_directories(
    Import/glfw/include
)

function(AddGLFW)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(Import/glfw)
endfunction(AddGLFW)

AddGLFW()

add_subdirectory(Import/WebGPU-distribution-wgpu)
add_subdirectory(Import/glfw3webgpu)
add_subdirectory(Import/glm)
add_subdirectory(Import/tinyobjloader)

add_library(WebGPUCPP WebGPUCPP.cpp)
target_link_libraries(WebGPUCPP webgpu)

option(DEV_MODE "Set up development helper settings" ON)

add_executable(App
    main.cpp
    ResourceLoading.h
    ResourceLoading.cpp
    VertexAttributes.h
)

target_link_libraries(App WebGPUCPP glfw webgpu glfw3webgpu glm::glm tinyobjloader)
target_copy_webgpu_binaries(App)

if(DEV_MODE)
    # In dev mode, we load resources from the source tree, so that when we
    # dynamically edit resources (like shaders), these are correctly
    # versionned.
    target_compile_definitions(App PRIVATE
        RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
    )
else()
    # In release mode, we just load resources relatively to wherever the
    # executable is launched from, so that the binary is portable
    target_compile_definitions(App PRIVATE
        RESOURCE_DIR="./resources"
    )
endif()
