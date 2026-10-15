#pragma once
#include "shader/shader_asset.h"

struct ShaderGLState
{
    int activeTexture=0, uniformBuffer=0, textures[16]{}, cubes[16]{}, buffers[3]{};
    ShaderGLState();
    ~ShaderGLState();
};
struct ShaderProgram
{
    struct TextureBinding { unsigned slot=0, unit=0, target=0x0DE1; };
    std::vector<TextureBinding> textures;
    ShaderAsset asset;
    std::string path, label;
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
