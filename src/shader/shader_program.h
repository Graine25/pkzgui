#pragma once
#include "shader/shader_asset.h"

struct ShaderGLState
{
    int activeTexture=0, uniformBuffer=0, textures[16]{}, buffers[3]{};
    ShaderGLState();
    ~ShaderGLState();
};
struct ShaderProgram
{
    ShaderAsset asset;
    std::string path;
    bool enabled=false, captured=false, capturedCamera=false;
    unsigned int program=0, buffers[3]{};
    ShaderConstants capturedConstants;
    ShaderProgram()=default;
    ShaderProgram(const ShaderProgram&)=delete;
    ShaderProgram& operator=(const ShaderProgram&)=delete;
    ~ShaderProgram();
    void Load(const ShaderAsset& source);
    void LoadConstants(const std::string& path);
    void Bind(const float* mvp,const float* view,const float* color,bool alphaCutout, uint32_t signs=0, uint32_t alphaSigns=0);
    void Clear();
};
