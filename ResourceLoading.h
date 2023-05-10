#pragma once

#include "VertexAttributes.h"

#include "webgpu/webgpu.hpp"

#include <filesystem>
#include <stdint.h>
#include <vector>

bool loadGeometry(std::filesystem::path const & path, std::vector<float> & pointData, std::vector<uint16_t> & indexData, int dimensions);
bool loadGeometryFromObj(std::filesystem::path const & path, std::vector<VertexAttributes> & vertexData);
wgpu::ShaderModule loadShaderModule(std::filesystem::path const & path, wgpu::Device device);
