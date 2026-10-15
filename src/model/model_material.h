#pragma once
#include "Resource/ResourceHeader.h"
#include "Package/PKPackage.h"
#include <optional>
#include <vector>

struct ModelAttribute
{
    uint16_t stream = 0, offset = 0;
    uint32_t type = 0;
    uint8_t usage = 0, index = 0;
};
struct ModelDeclaration
{
    uint32_t format = 0, slot = 0, set = 0;
    std::vector<ModelAttribute> attributes;
};
struct ModelShaderRecord
{
    size_t virtualOffset = 0, virtualSize = 0, physicalOffset = 0, physicalSize = 0, headerSize = 0;
};
struct ModelPass
{
    uint32_t slot = 0, set = 0, format = 0;
    ModelShaderRecord vertex, pixel;
};
struct ModelSampler
{
    uint32_t crc = 0;
    uint8_t addressMode = 0;
    std::string sourcePath;
    std::optional<CMChunkResourceHeader> texture;
};
struct ModelMaterial
{
    std::string name;
    const CMChunk* source = nullptr;
    uint32_t flags = 0;
    std::vector<ModelPass> passes;
    std::vector<ModelSampler> samplers;
    bool alphaCutout = false;
    uint32_t textureCRC = 0;
    uint8_t uvSet = 0, addressMode = 0;
    float color[4] = {1, 1, 1, 1};
    std::optional<CMChunkResourceHeader> texture;
    std::vector<ModelDeclaration> declarations;
};

const CMChunk* FindModelResource(const PKPackage& package, const CMChunkResourceHeader& resource);
std::vector<ModelMaterial> LoadModelMaterials(const PKPackage& package, const CMChunkResourceHeader& resource);
const ModelDeclaration* ModelVertexDeclaration(const ModelMaterial& material, uint32_t format);

std::vector<uint8_t> ModelShaderContainer(const ModelMaterial& material, const ModelShaderRecord& record);
void ExportModelShaders(const ModelMaterial& material, const std::string& path);
