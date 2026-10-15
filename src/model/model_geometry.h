#pragma once
#include "model/model_material.h"

struct ModelMesh
{
    std::vector<float> positions;   // xyz
    std::vector<float> normals;     // xyz
    std::vector<uint32_t> indices;
    std::vector<float> texcoords;
    bool hasUV = false, integerUV = false;
    uint32_t material = 0;
};

std::vector<ModelMesh> LoadModelGeometry(const PKPackage& package, const CMChunkResourceHeader& resource,
    const std::vector<ModelMaterial>& materials);
