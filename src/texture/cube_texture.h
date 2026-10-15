#pragma once
#include "Package/CMChunk.h"
#include <array>
#include <vector>

struct CubeTexture
{
    uint32_t size=0, format=0;
    std::array<std::vector<uint8_t>,6> faces;
};
CubeTexture DecodeCubeTexture(const CMChunk& data);
