#include "ResourceLoading.h"

#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#include "webgpu/webgpu.hpp"

#include <iostream>
#include <vector>

template <typename IntT, typename SizeT>
constexpr IntT Align(IntT n, SizeT alignment)
{
    if (alignment <= 1)
    {
        return n;
    }

    SizeT alignmentMask = alignment - 1;
    return IntT((n + alignmentMask) & ~alignmentMask);
}

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
    requiredLimits.limits.maxBufferSize = 16384 * sizeof(float);
    requiredLimits.limits.maxVertexBufferArrayStride = 5 * sizeof(float);
    requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
    requiredLimits.limits.maxInterStageShaderComponents = 3;
    requiredLimits.limits.maxBindGroups = 1;
    requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
    requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4;

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
	std::cout << "Swapchain format: " << swapChainFormat << std::endl;

    wgpu::ShaderModule shaderModule = loadShaderModule(RESOURCE_DIR "/shader.wgsl", device);

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

    wgpu::BindGroupLayoutEntry bindingLayout = wgpu::Default;
    bindingLayout.binding = 0;
    bindingLayout.visibility = wgpu::ShaderStage::Vertex;
    bindingLayout.buffer.type = wgpu::BufferBindingType::Uniform;
    bindingLayout.buffer.minBindingSize = sizeof(float);

    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries = &bindingLayout;
    wgpu::BindGroupLayout bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

    // Create the pipeline layout
    wgpu::PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)&bindGroupLayout;
    wgpu::PipelineLayout layout = device.createPipelineLayout(layoutDesc);
    pipelineDesc.layout = layout;

    wgpu::RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);

    std::vector<float> vertexData;
    std::vector<uint16_t> indexData;

    loadGeometry(RESOURCE_DIR "/webgpu.txt", vertexData, indexData);

    int vertexCount = static_cast<int>(vertexData.size() / 5);
    int indexCount = static_cast<int>(indexData.size());

    wgpu::BufferDescriptor vertexBufferDesc = {};
    vertexBufferDesc.size = vertexData.size() * sizeof(float);
    vertexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
    vertexBufferDesc.mappedAtCreation = false;
    wgpu::Buffer vertexBuffer = device.createBuffer(vertexBufferDesc);
    queue.writeBuffer(vertexBuffer, 0, (void *)vertexData.data(), vertexBufferDesc.size);

    wgpu::BufferDescriptor indexBufferDesc = {};
    indexBufferDesc.size = Align(indexData.size() * sizeof(uint16_t), 4);
    indexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
    indexBufferDesc.mappedAtCreation = false;
    wgpu::Buffer indexBuffer = device.createBuffer(indexBufferDesc);
    queue.writeBuffer(indexBuffer, 0, (void *)indexData.data(), indexBufferDesc.size);

    wgpu::BufferDescriptor uniformBufferDesc{};
    uniformBufferDesc.size = sizeof(float);
    uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    uniformBufferDesc.mappedAtCreation = false;
    wgpu::Buffer uniformBuffer = device.createBuffer(uniformBufferDesc);

    wgpu::BindGroupEntry bindGroupEntry{};
    bindGroupEntry.binding = 0;
    bindGroupEntry.buffer = uniformBuffer;
    bindGroupEntry.offset = 0;
    bindGroupEntry.size = sizeof(float);

    wgpu::BindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.layout = bindGroupLayout;
    bindGroupDesc.entryCount = bindGroupLayoutDesc.entryCount;
    bindGroupDesc.entries = &bindGroupEntry;
    wgpu::BindGroup bindGroup = device.createBindGroup(bindGroupDesc);

    float currentTime = 0.f;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        wgpu::TextureView nextTexture = swapChain.getCurrentTextureView();

        if (!nextTexture)
        {
            std::cerr << "Cannot acquire next swap chain texture" << std::endl;
            break;
        }

        queue.writeBuffer(uniformBuffer, 0, &currentTime, sizeof(float));

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

        encoder.setBindGroup(0, bindGroup, 0, nullptr);

        encoder.setPipeline(pipeline);
        encoder.setVertexBuffer(0, vertexBuffer, 0, vertexData.size() * sizeof(float));
        encoder.setIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint16, 0, indexData.size() * sizeof(uint16_t));
        encoder.drawIndexed(indexCount, 1, 0, 0, 0);

        encoder.end();

        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {};
        cmdBufferDescriptor.label = "Command buffer";
        wgpu::CommandBuffer command = commandEncoder.finish(cmdBufferDescriptor);
        queue.submit(1, &command);

        swapChain.present();

        currentTime += .01f;
    }

    vertexBuffer.destroy();
    indexBuffer.destroy();
    uniformBuffer.destroy();

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
