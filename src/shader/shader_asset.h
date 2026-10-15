#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct ShaderConstant
{
    bool pixel = false;
    uint32_t index = 0, count = 0;
    std::string name;
};
struct ShaderAsset
{
    std::string vertexHLSL, pixelHLSL, vertexGLSL, pixelGLSL;
    std::vector<ShaderConstant> constants;
};
struct ShaderConstants
{
    std::array<float, 1024> vertex{}, pixel{};
    std::array<uint32_t, 256> shared{};
};
ShaderAsset ReadShaderAsset(const std::string& path);
void WriteShaderAsset(const ShaderAsset& asset, const std::string& path);
ShaderAsset CompileGameShader(const std::vector<uint8_t>& vertex, const std::vector<uint8_t>& pixel);
ShaderAsset CompileShaderEdits(const ShaderAsset& source, const std::string& vertexPath, const std::string& pixelPath);
bool ShaderCompilerAvailable();
