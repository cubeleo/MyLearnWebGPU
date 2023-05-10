#include "ResourceLoading.h"
#include "VertexAttributes.h"

#include "glfw3webgpu.h"
#include "GLFW/glfw3.h"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include "webgpu/webgpu.hpp"

#include <array>
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

struct MyUniforms
{
    glm::mat4x4 clipFromView;
    glm::mat4x4 viewFromWorld;
    glm::mat4x4 worldFromObject;
    std::array<float, 4> color;
    float time;
    float _pad[3];
};

static_assert(sizeof(MyUniforms) % 16 == 0);

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

    // Non-glfw-specific way to init webgpu on Win32:
    // wgpu::SurfaceDescriptorFromWindowsHWND surfaceDescriptorFromWindowsHWND{};
    // surfaceDescriptorFromWindowsHWND.hinstance = GetModuleHandle(NULL);
    // surfaceDescriptorFromWindowsHWND.hwnd = glfwGetWin32Window(window);
    // surfaceDescriptorFromWindowsHWND.chain =
    // {
    //     .next = nullptr,
    //     .sType = WGPUSType_SurfaceDescriptorFromWindowsHWND
    // };
    // wgpu::SurfaceDescriptor surfaceDescriptor;
    // surfaceDescriptor.nextInChain = (const WGPUChainedStruct*)&surfaceDescriptorFromWindowsHWND;

    // wgpu::Surface surface = instance.createSurface(surfaceDescriptor);

	wgpu::Surface surface = glfwGetWGPUSurface(instance, window);

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
    requiredLimits.limits.maxVertexAttributes = 3;
    requiredLimits.limits.maxVertexBuffers = 1;
    requiredLimits.limits.maxBufferSize = 16 * 1024 * 1024;
    requiredLimits.limits.maxVertexBufferArrayStride = 9 * sizeof(float);
    requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
    requiredLimits.limits.maxInterStageShaderComponents = 6;
    requiredLimits.limits.maxBindGroups = 1;
    requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
    requiredLimits.limits.maxUniformBufferBindingSize = 256;
    requiredLimits.limits.maxTextureDimension2D = 4096;
    requiredLimits.limits.maxTextureArrayLayers = 1;

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

    wgpu::TextureFormat depthTextureFormat = wgpu::TextureFormat::Depth24Plus;

    wgpu::TextureDescriptor depthTextureDesc;
    depthTextureDesc.dimension = wgpu::TextureDimension::_2D;
    depthTextureDesc.format = depthTextureFormat;
    depthTextureDesc.mipLevelCount = 1;
    depthTextureDesc.sampleCount = 1;
    depthTextureDesc.size = {640, 480, 1};
    depthTextureDesc.usage = wgpu::TextureUsage::RenderAttachment;
    depthTextureDesc.viewFormatCount = 1;
    depthTextureDesc.viewFormats = (WGPUTextureFormat*)&depthTextureFormat;
    wgpu::Texture depthTexture = device.createTexture(depthTextureDesc);

    wgpu::TextureViewDescriptor depthTextureViewDesc;
    depthTextureViewDesc.aspect = wgpu::TextureAspect::DepthOnly;
    depthTextureViewDesc.baseArrayLayer = 0;
    depthTextureViewDesc.arrayLayerCount = 1;
    depthTextureViewDesc.baseMipLevel = 0;
    depthTextureViewDesc.mipLevelCount = 1;
    depthTextureViewDesc.dimension = wgpu::TextureViewDimension::_2D;
    depthTextureViewDesc.format = depthTextureFormat;
    wgpu::TextureView depthTextureView = depthTexture.createView(depthTextureViewDesc);

    wgpu::ShaderModule shaderModule = loadShaderModule(RESOURCE_DIR "/shader.wgsl", device);

    wgpu::RenderPipelineDescriptor pipelineDesc;

    wgpu::VertexAttribute vertexAttributes[3];
    vertexAttributes[0].shaderLocation = 0;
    vertexAttributes[0].format = wgpu::VertexFormat::Float32x3;
    vertexAttributes[0].offset = 0;
    vertexAttributes[1].shaderLocation = 1;
    vertexAttributes[1].format = wgpu::VertexFormat::Float32x3;
    vertexAttributes[1].offset = 3 * sizeof(float);
    vertexAttributes[2].shaderLocation = 2;
    vertexAttributes[2].format = wgpu::VertexFormat::Float32x3;
    vertexAttributes[2].offset = 6 * sizeof(float);

    wgpu::VertexBufferLayout vertexBufferLayout;
    vertexBufferLayout.attributeCount = 3;
    vertexBufferLayout.attributes = &vertexAttributes[0];
    vertexBufferLayout.arrayStride = 9 * sizeof(float);
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

    wgpu::DepthStencilState depthStencilState = wgpu::Default;
    depthStencilState.depthCompare = wgpu::CompareFunction::Less;
    depthStencilState.depthWriteEnabled = true;
    depthStencilState.format = depthTextureFormat;
    depthStencilState.stencilReadMask = 0;
    depthStencilState.stencilWriteMask = 0;
    pipelineDesc.depthStencil = &depthStencilState;

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
    bindingLayout.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
    bindingLayout.buffer.type = wgpu::BufferBindingType::Uniform;
    bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);

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

    std::vector<uint16_t> indexData = {1, 2, 3};

    std::vector<VertexAttributes> vertexData;
    bool success = loadGeometryFromObj(RESOURCE_DIR "/pyramid.obj", vertexData);
    if (!success) {
        std::cerr << "Could not load geometry!" << std::endl;
        return 1;
    }

    int indexCount = static_cast<int>(vertexData.size()); //static_cast<int>(indexData.size());

    wgpu::BufferDescriptor vertexBufferDesc = {};
    vertexBufferDesc.size = vertexData.size() * sizeof(VertexAttributes);
    vertexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
    vertexBufferDesc.mappedAtCreation = false;
    wgpu::Buffer vertexBuffer = device.createBuffer(vertexBufferDesc);
    queue.writeBuffer(vertexBuffer, 0, (void *)vertexData.data(), vertexBufferDesc.size);

    // wgpu::BufferDescriptor indexBufferDesc = {};
    // indexBufferDesc.size = Align(indexData.size() * sizeof(uint16_t), 4);
    // indexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
    // indexBufferDesc.mappedAtCreation = false;
    // wgpu::Buffer indexBuffer = device.createBuffer(indexBufferDesc);
    // queue.writeBuffer(indexBuffer, 0, (void *)indexData.data(), indexBufferDesc.size);

    MyUniforms myUniforms;
    myUniforms.time = 0.f;
    myUniforms.color = { 0.0f, 1.0f, 0.4f, 1.0f };

    wgpu::BufferDescriptor uniformBufferDesc{};
    uniformBufferDesc.size = sizeof(myUniforms);
    uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    uniformBufferDesc.mappedAtCreation = false;
    wgpu::Buffer uniformBuffer = device.createBuffer(uniformBufferDesc);

    wgpu::BindGroupEntry bindGroupEntry{};
    bindGroupEntry.binding = 0;
    bindGroupEntry.buffer = uniformBuffer;
    bindGroupEntry.offset = 0;
    bindGroupEntry.size = sizeof(myUniforms);

    wgpu::BindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.layout = bindGroupLayout;
    bindGroupDesc.entryCount = bindGroupLayoutDesc.entryCount;
    bindGroupDesc.entries = &bindGroupEntry;
    wgpu::BindGroup bindGroup = device.createBindGroup(bindGroupDesc);

    // Model matrix
    float angle1 = 2.0f;
    glm::mat4x4 S = glm::scale(glm::mat4x4(1.0), glm::vec3(0.3f));
    glm::mat4x4 T1 = glm::translate(glm::mat4x4(1.0), glm::vec3(0.5, 0.0, 0.0));
    glm::mat4x4 R1 = glm::rotate(glm::mat4x4(1.0), angle1, glm::vec3(0.0, 0.0, 1.0));
    myUniforms.worldFromObject = R1 * T1 * S;

    // View matrix
    float angle2 = 3.0f * 3.14159f / 4.0f;
    glm::vec3 focalPoint(0.0, 0.0, -2.0);
    glm::mat4x4 R2 = glm::rotate(glm::mat4x4(1.0), -angle2, glm::vec3(1.0, 0.0, 0.0));
    glm::mat4x4 T2 = glm::translate(glm::mat4x4(1.0), -focalPoint);
    myUniforms.viewFromWorld = T2 * R2;

    // Projection matrix
    float ratio = 640.0f / 480.0f;
    float near = 0.01f;
    float far = 100.0f;
    float focalLength = 2.0f;
    float fov = 2 * glm::atan(1 / focalLength);
    myUniforms.clipFromView = glm::perspective(fov, ratio, near, far);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        wgpu::TextureView nextTexture = swapChain.getCurrentTextureView();

        if (!nextTexture)
        {
            std::cerr << "Cannot acquire next swap chain texture" << std::endl;
            break;
        }

        angle1 = myUniforms.time;
        R1 = glm::rotate(glm::mat4x4(1.0), angle1, glm::vec3(0.0, 0.0, 1.0));
        myUniforms.worldFromObject = R1 * T1 * S;
        queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, worldFromObject), &myUniforms.worldFromObject, sizeof(MyUniforms::worldFromObject));

        queue.writeBuffer(uniformBuffer, 0, &myUniforms, sizeof(MyUniforms));

        wgpu::CommandEncoderDescriptor commandEncoderDesc{};
        commandEncoderDesc.label = "My command encoder";
        wgpu::CommandEncoder commandEncoder = device.createCommandEncoder(commandEncoderDesc);

        wgpu::RenderPassDescriptor renderPassDesc{};

        wgpu::RenderPassColorAttachment renderPassColorAttachment{};
        renderPassColorAttachment.view = nextTexture;
        renderPassColorAttachment.resolveTarget = nullptr;
        renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
        renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
        renderPassColorAttachment.clearValue = WGPUColor{ .05, .05, .05, 1.0 };

        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;

        wgpu::RenderPassDepthStencilAttachment depthStencilAttachment;
        depthStencilAttachment.view = depthTextureView;
        depthStencilAttachment.depthClearValue = 100.0f;
        depthStencilAttachment.depthLoadOp = wgpu::LoadOp::Clear;
        depthStencilAttachment.depthStoreOp = wgpu::StoreOp::Store;
        depthStencilAttachment.depthReadOnly = false;
        depthStencilAttachment.stencilClearValue = 0;
        depthStencilAttachment.stencilLoadOp = wgpu::LoadOp::Clear;
        depthStencilAttachment.stencilStoreOp = wgpu::StoreOp::Store;
        depthStencilAttachment.stencilReadOnly = false;
        renderPassDesc.depthStencilAttachment = &depthStencilAttachment;

        renderPassDesc.timestampWriteCount = 0;
        renderPassDesc.timestampWrites = nullptr;

        wgpu::RenderPassEncoder encoder = commandEncoder.beginRenderPass(renderPassDesc);

        encoder.setBindGroup(0, bindGroup, 0, nullptr);

        encoder.setPipeline(pipeline);
        encoder.setVertexBuffer(0, vertexBuffer, 0, vertexData.size() * sizeof(VertexAttributes));
        encoder.draw(indexCount, 1, 0, 0);
        // encoder.setIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint16, 0, indexData.size() * sizeof(uint16_t));
        // encoder.drawIndexed(indexCount, 1, 0, 0, 0);

        encoder.end();

        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {};
        cmdBufferDescriptor.label = "Command buffer";
        wgpu::CommandBuffer command = commandEncoder.finish(cmdBufferDescriptor);
        queue.submit(1, &command);

        swapChain.present();

        myUniforms.time += .01f;
    }

    vertexBuffer.destroy();
    // indexBuffer.destroy();
    uniformBuffer.destroy();

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
