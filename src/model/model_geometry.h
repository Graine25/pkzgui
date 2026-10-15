#pragma once
#include "model/model_material.h"
#include <array>

struct ModelMesh
{
    std::vector<float> positions;   // xyz
    std::vector<float> normals;     // xyz
    std::vector<uint32_t> indices;
    std::vector<float> texcoords, tangents, colors;
    std::array<std::vector<float>,4> shaderUV;
    std::array<bool,4> hasShaderUV{};
    std::string shaderVertexError;
    bool hasUV = false, integerUV = false;
    uint32_t material = 0;
};

std::vector<ModelMesh> LoadModelGeometry(const PKPackage& package, const CMChunkResourceHeader& resource,
    const std::vector<ModelMaterial>& materials);
