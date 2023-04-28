cmake_minimum_required(VERSION 3.25)
project(LearnWebGPU C CXX)
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

add_library(WebGPUCPP WebGPUCPP.cpp)
target_link_libraries(WebGPUCPP webgpu)

add_executable(App
    main.cpp
)

target_link_libraries(App WebGPUCPP glfw webgpu)
target_copy_webgpu_binaries(App)
