#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#include "webgpu/webgpu.hpp"

#include <iostream>
#include <vector>

const char * shaderSource = R"(
@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32>
{
    var p = vec2<f32>(0.0, 0.0);

    if (in_vertex_index == 0u)
    {
        p = vec2<f32>(-0.5, -0.5);
    }
    else if (in_vertex_index == 1u)
    {
        p = vec2<f32>(0.5, -0.5);
    }
    else
    {
        p = vec2<f32>(0.0, 0.5);
    }

    return vec4<f32>(p, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4<f32>
{
    return vec4<f32>(0.0, 0.4, 1.0, 1.0);
}
)";

int main(int argc, char ** argv)
{
    if (!glfwInit())
    {
        std::cerr << "Could not initialize GLFW!\n";
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow * window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
    if (!window)
    {
        std::cerr << "Could not open window!\n";
        glfwTerminate();
        return 1;
    }

    wgpu::InstanceDescriptor desc{};
    wgpu::Instance instance = wgpu::createInstance(desc);

    if (!instance)
    {
        std::cerr << "Could not initialize WebGPU!\n";
        return 1;
    }

    wgpu::SurfaceDescriptorFromWindowsHWND surfaceDescriptorFromWindowsHWND{};
    surfaceDescriptorFromWindowsHWND.hinstance = GetModuleHandle(NULL);
    surfaceDescriptorFromWindowsHWND.hwnd = glfwGetWin32Window(window);
    surfaceDescriptorFromWindowsHWND.chain =
    {
        .next = nullptr,
        .sType = WGPUSType_SurfaceDescriptorFromWindowsHWND
    };
    wgpu::SurfaceDescriptor surfaceDescriptor;
    surfaceDescriptor.nextInChain = (const WGPUChainedStruct*)&surfaceDescriptorFromWindowsHWND;

    wgpu::Surface surface = instance.createSurface(surfaceDescriptor);

    wgpu::RequestAdapterOptions adapterOptions{};
    adapterOptions.compatibleSurface = surface;
    wgpu::Adapter adapter = instance.requestAdapter(adapterOptions);

    // {
    //     std::vector<wgpu::FeatureName> features;

    //     // Call the function a first time with a null return address, just to get
    //     // the entry count.
    //     size_t featureCount = adapter.enumerateFeatures(nullptr);

    //     // Allocate memory (could be a new, or a malloc() if this were a C program)
    //     features.resize(featureCount);

    //     // Call the function a second time, with a non-null return address
    //     adapter.enumerateFeatures(features.data());

    //     std::cout << "Adapter features:" << std::endl;
    //     for (auto f : features)
    //     {
    //         std::cout << " - " << f << std::endl;
    //     }
    // }

    wgpu::DeviceDescriptor deviceDesc{};
    deviceDesc.label = "Doteki Device"; // anything works here, that's your call
    deviceDesc.requiredFeaturesCount = 0; // we do not require any specific feature
    deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
    deviceDesc.defaultQueue.label = "The default queue";
    wgpu::Device device = adapter.requestDevice(deviceDesc);

    std::cout << "Got device: " << device << std::endl;

    auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */)
    {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
    };
    wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData */);

    wgpu::Queue queue = device.getQueue();

    std::cout << "Got queue: " << queue << std::endl;


    wgpu::SwapChainDescriptor swapChainDesc{};
    swapChainDesc.width = 640;
    swapChainDesc.height = 480;
    wgpu::TextureFormat swapChainFormat = surface.getPreferredFormat(adapter);
    swapChainDesc.format = swapChainFormat;
    swapChainDesc.usage = wgpu::TextureUsage::RenderAttachment;
    swapChainDesc.presentMode = wgpu::PresentMode::Fifo;
    wgpu::SwapChain swapChain = device.createSwapChain(surface, swapChainDesc);
    std::cout << "Swapchain: " << swapChain << std::endl;

    wgpu::ShaderModuleDescriptor shaderDesc;
    #ifdef WEBGPU_BACKEND_WGPU
    shaderDesc.hintCount = 0;
    shaderDesc.hints = nullptr;
    #endif

    wgpu::ShaderModuleWGSLDescriptor shaderCodeDesc;
    // Set the chained struct's header
    shaderCodeDesc.chain.next = nullptr;
    shaderCodeDesc.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
    // Connect the chain
    shaderDesc.nextInChain = &shaderCodeDesc.chain;

    shaderCodeDesc.code = shaderSource;

    wgpu::ShaderModule shaderModule = device.createShaderModule(shaderDesc);

    wgpu::RenderPipelineDescriptor pipelineDesc;
    pipelineDesc.vertex.bufferCount = 0;
    pipelineDesc.vertex.buffers = nullptr;
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;

    pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    pipelineDesc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
    pipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;
    pipelineDesc.primitive.cullMode = wgpu::CullMode::None;

    wgpu::FragmentState fragmentState;
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;
    pipelineDesc.fragment = &fragmentState;

    pipelineDesc.depthStencil = nullptr;

    wgpu::BlendState blendState;
    blendState.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
    blendState.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
    blendState.color.operation = wgpu::BlendOperation::Add;
    blendState.alpha.srcFactor = wgpu::BlendFactor::Zero;
    blendState.alpha.dstFactor = wgpu::BlendFactor::One;
    blendState.alpha.operation = wgpu::BlendOperation::Add;

    wgpu::ColorTargetState colorTarget;
    colorTarget.format = swapChainFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = wgpu::ColorWriteMask::All;

    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    pipelineDesc.multisample.count = 1;
    pipelineDesc.multisample.mask = ~0u;
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    pipelineDesc.layout = nullptr;

    wgpu::RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        wgpu::TextureView nextTexture = swapChain.getCurrentTextureView();

        if (!nextTexture)
        {
            std::cerr << "Cannot acquire next swap chain texture" << std::endl;
            break;
        }

        wgpu::CommandEncoderDescriptor commandEncoderDesc{};
        commandEncoderDesc.label = "My command encoder";
        wgpu::CommandEncoder commandEncoder = device.createCommandEncoder(commandEncoderDesc);

        wgpu::RenderPassDescriptor renderPassDesc{};

        wgpu::RenderPassColorAttachment renderPassColorAttachment{};
        renderPassColorAttachment.view = nextTexture;
        renderPassColorAttachment.resolveTarget = nullptr;
        renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
        renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
        renderPassColorAttachment.clearValue = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };

        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;

        renderPassDesc.depthStencilAttachment = nullptr;

        renderPassDesc.timestampWriteCount = 0;
        renderPassDesc.timestampWrites = nullptr;

        wgpu::RenderPassEncoder renderPassEncoder = commandEncoder.beginRenderPass(renderPassDesc);

        renderPassEncoder.setPipeline(pipeline);
        renderPassEncoder.draw(3, 1, 0, 0);

        renderPassEncoder.end();

        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {};
        cmdBufferDescriptor.label = "Command buffer";
        wgpu::CommandBuffer command = commandEncoder.finish(cmdBufferDescriptor);
        queue.submit(1, &command);

        swapChain.present();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
