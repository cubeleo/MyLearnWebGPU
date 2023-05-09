#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#include "webgpu/webgpu.hpp"

#include <iostream>
#include <vector>

const char * shaderSource = R"(
struct VertexInput
{
    @location(0) position: vec2<f32>,
    @location(1) color: vec3<f32>,
};

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec3<f32>,
};

@vertex
fn vs_main(in: VertexInput) -> VertexOutput
{
    var out: VertexOutput;
    out.position = vec4<f32>(in.position, 0.0, 1.0);
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32>
{
    return vec4<f32>(in.color, 1.0);
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

    wgpu::SupportedLimits supportedLimits;

    adapter.getLimits(&supportedLimits);
    std::cout << "adapter.maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;

    wgpu::RequiredLimits requiredLimits = wgpu::Default;
    requiredLimits.limits.maxVertexAttributes = 2;
    requiredLimits.limits.maxVertexBuffers = 1;
    requiredLimits.limits.maxBufferSize = 6 * 5 * sizeof(float);
    requiredLimits.limits.maxVertexBufferArrayStride = 5 * sizeof(float);
    requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
    requiredLimits.limits.maxInterStageShaderComponents = 3;

    wgpu::DeviceDescriptor deviceDesc{};
    deviceDesc.label = "Doteki Device";
    deviceDesc.requiredFeaturesCount = 0;
    deviceDesc.requiredLimits = &requiredLimits;
    deviceDesc.defaultQueue.label = "The default queue";
    wgpu::Device device = adapter.requestDevice(deviceDesc);

    std::cout << "Got device: " << device << std::endl;

    device.getLimits(&supportedLimits);
    std::cout << "device.maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;

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

    wgpu::VertexAttribute vertexAttributes[2];
    vertexAttributes[0].shaderLocation = 0;
    vertexAttributes[0].format = wgpu::VertexFormat::Float32x2;
    vertexAttributes[0].offset = 0;
    vertexAttributes[1].shaderLocation = 1;
    vertexAttributes[1].format = wgpu::VertexFormat::Float32x3;
    vertexAttributes[1].offset = 2 * sizeof(float);

    wgpu::VertexBufferLayout vertexBufferLayout;
    vertexBufferLayout.attributeCount = 2;
    vertexBufferLayout.attributes = &vertexAttributes[0];
    vertexBufferLayout.arrayStride = 5 * sizeof(float);
    vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
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

    std::vector<float> vertexData =
    {
        -0.5, -0.5, 1.0, 0.0, 0.0,
        +0.5, -0.5, 0.0, 1.0, 0.0,
        +0.0,   +0.5, 0.0, 0.0, 1.0,

        -0.55f, -0.5, 1.0, 1.0, 0.0,
        -0.05f, +0.5, 1.0, 0.0, 1.0,
        -0.55f, +0.5, 0.0, 1.0, 1.0
    };
    int vertexCount = static_cast<int>(vertexData.size() / 5);

    wgpu::BufferDescriptor bufferDesc = {};
    bufferDesc.size = vertexData.size() * sizeof(float);
    bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
    bufferDesc.mappedAtCreation = false;
    wgpu::Buffer vertexBuffer = device.createBuffer(bufferDesc);

    queue.writeBuffer(vertexBuffer, 0, (void *)vertexData.data(), bufferDesc.size);

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
        renderPassColorAttachment.clearValue = WGPUColor{ 0.1, 0.1, 0.2, 1.0 };

        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;

        renderPassDesc.depthStencilAttachment = nullptr;

        renderPassDesc.timestampWriteCount = 0;
        renderPassDesc.timestampWrites = nullptr;

        wgpu::RenderPassEncoder encoder = commandEncoder.beginRenderPass(renderPassDesc);

        encoder.setPipeline(pipeline);
        encoder.setVertexBuffer(0, vertexBuffer, 0, vertexData.size() * sizeof(float));
        encoder.draw(vertexCount, 1, 0, 0);

        encoder.end();

        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {};
        cmdBufferDescriptor.label = "Command buffer";
        wgpu::CommandBuffer command = commandEncoder.finish(cmdBufferDescriptor);
        queue.submit(1, &command);

        swapChain.present();
    }

    vertexBuffer.destroy();

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
