#include "shader/shader_asset.h"
#include "platform/file_io.h"
#include <cstring>
#include <stdexcept>

namespace {
constexpr size_t MaxAssetBytes = 20 * 1024 * 1024;
void Word(std::vector<uint8_t>& data, uint32_t n)
{
    for (int i=3; i>=0; --i) data.push_back(uint8_t(n >> (8*i)));
}
void String(std::vector<uint8_t>& data, const std::string& text)
{
    if (text.find('\0') != std::string::npos) throw std::runtime_error("Shader source contains a null byte.");
    if (text.size() > 4*1024*1024) throw std::runtime_error("Shader source exceeds 4 MiB.");
    Word(data,uint32_t(text.size())); data.insert(data.end(),text.begin(),text.end());
}
struct Reader {
    const std::vector<uint8_t>& data;
    size_t at = 8;
    uint32_t word() {
        if (at > data.size() || data.size()-at < 4) throw std::runtime_error("Truncated GSX shader asset.");
        uint32_t n=0; for(int i=0;i<4;++i) n=(n<<8)|data[at++];return n;
    }
    std::string string() {
        const auto n=word();if(n>4*1024*1024 || n>data.size()-at)throw std::runtime_error("Invalid GSX shader source length.");
        std::string text(data.begin()+at,data.begin()+at+n);at+=n;
        if (text.find('\0') != std::string::npos) throw std::runtime_error("Shader source contains a null byte.");
        return text;
    }
};
}
ShaderAsset ReadShaderAsset(const std::string& path)
{
    const auto file = std::filesystem::u8path(path);
    if (std::filesystem::file_size(file) > MaxAssetBytes) throw std::runtime_error("GSX shader asset exceeds 20 MiB.");
    const auto data=ReadFile(file);
    if(data.size()<8 || std::memcmp(data.data(),"GSXSHD01",8))throw std::runtime_error("Not a GSX shader preview asset.");
    Reader r{data}; ShaderAsset asset;
    asset.vertexHLSL=r.string();asset.pixelHLSL=r.string();asset.vertexGLSL=r.string();asset.pixelGLSL=r.string();
    const auto count=r.word();if(count>512)throw std::runtime_error("Too many shader constants.");
    for(uint32_t i=0;i<count;++i){ShaderConstant c;const auto stage=r.word();if(stage>1)throw std::runtime_error("Invalid shader stage.");c.pixel=stage!=0;c.index=r.word();c.count=r.word();c.name=r.string();
        if(c.name.size()>1024 || !c.count || c.index>=256 || c.count>256-c.index)throw std::runtime_error("Shader constant register is out of range.");asset.constants.push_back(std::move(c));}
    if(r.at!=data.size() || asset.vertexGLSL.empty() || asset.pixelGLSL.empty())throw std::runtime_error("Invalid GSX shader asset contents.");
    return asset;
}
void WriteShaderAsset(const ShaderAsset& asset,const std::string& path)
{
    if (asset.constants.size() > 512 || asset.vertexGLSL.empty() || asset.pixelGLSL.empty())
        throw std::runtime_error("Invalid GSX shader asset contents.");
    for (const auto& c : asset.constants)
        if (c.name.size() > 1024 || !c.count || c.index >= 256 || c.count > 256 - c.index)
            throw std::runtime_error("Shader constant register is out of range.");
    std::vector<uint8_t> data={'G','S','X','S','H','D','0','1'};
    String(data,asset.vertexHLSL);String(data,asset.pixelHLSL);String(data,asset.vertexGLSL);String(data,asset.pixelGLSL);
    Word(data,uint32_t(asset.constants.size()));
    for(const auto& c:asset.constants){Word(data,c.pixel);Word(data,c.index);Word(data,c.count);String(data,c.name);}
    if (data.size() > MaxAssetBytes) throw std::runtime_error("GSX shader asset exceeds 20 MiB.");
    WriteFile(std::filesystem::u8path(path),data);
    WriteFile(std::filesystem::u8path(path+".vert.hlsl"),std::vector<uint8_t>(asset.vertexHLSL.begin(),asset.vertexHLSL.end()));
    WriteFile(std::filesystem::u8path(path+".frag.hlsl"),std::vector<uint8_t>(asset.pixelHLSL.begin(),asset.pixelHLSL.end()));
}
#ifndef GSX_SHADER_COMPILER
bool ShaderCompilerAvailable(){return false;}
ShaderAsset CompileGameShader(const std::vector<uint8_t>&,const std::vector<uint8_t>&){throw std::runtime_error("Build with GSX_SHADER_COMPILER=ON to convert game shaders. Converted .gsxshader previews can still be loaded.");}
ShaderAsset CompileShaderEdits(const ShaderAsset&,const std::string&,const std::string&){throw std::runtime_error("Build with GSX_SHADER_COMPILER=ON to compile HLSL edits.");}
#endif
