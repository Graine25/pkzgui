#include "texture/cube_texture.h"
#include "Graphics/XenosTexture.h"
#include <algorithm>
#include <stdexcept>

CubeTexture DecodeCubeTexture(const CMChunk& data)
{
    if(data.GetMaskedID()!=Texture_Data || data.version!=7 || data.data.size()<52)
        throw std::runtime_error("Expected an EOT v7 cube texture.");
    size_t at=0;
    const auto kind=data.Read<uint32_t>(at), flags=data.Read<uint32_t>(at);
    if(kind!=1)throw std::runtime_error("Shader expects a cube map, but the package texture has another kind.");
    if(flags&2)at+=12;
    if(flags&4)at+=4;
    if(at+44>data.data.size())throw std::runtime_error("Truncated cube texture header.");
    CubeTexture result;
    result.size=data.Read<uint32_t>(at);
    const auto mips=std::max(1u,data.Read<uint32_t>(at));
    at+=4;result.format=data.Read<uint32_t>(at);at+=28;
    if(!result.size || result.size>2048 || (result.size&(result.size-1)) || mips>13)
        throw std::runtime_error("Unsupported cube texture size or mip count.");
    const auto format=XenosTexture::D3DFormat::Decode(result.format);
    size_t stride=0;
    for(uint32_t mip=0;mip<mips;++mip){
        const auto edge=std::max(1u,result.size>>mip);
        stride+=XenosTexture::TiledSurfaceSize(edge,edge,format);
    }
    if(stride>data.data.size()/6 || data.data.size()-at!=stride*6)
        throw std::runtime_error("Unsupported or truncated cube face/mip layout.");
    for(size_t face=0;face<6;++face){
        const auto linear=XenosTexture::Untile(data.data.data()+at+face*stride,stride,result.size,result.size,format);
        result.faces[face]=XenosTexture::DecodeToRGBA8(linear,result.size,result.size,format);
    }
    return result;
}
