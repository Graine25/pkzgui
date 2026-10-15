#include "model/model_material.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <stdexcept>

namespace {
struct Reader
{
    const std::vector<uint8_t>& data;
    size_t at = 0;
    void skip(size_t n) { if (at > data.size() || n > data.size() - at) throw std::runtime_error("Truncated model material."); at += n; }
    uint32_t u32() { const size_t start = at; skip(4); return uint32_t(data[start]) << 24 | uint32_t(data[start+1]) << 16 | uint32_t(data[start+2]) << 8 | data[start+3]; }
    uint32_t word(size_t pos) { const size_t saved = at; at = pos; const auto v = u32(); at = saved; return v; }
    float number(size_t pos) { const auto v = word(pos); float f; std::memcpy(&f, &v, 4); if (!std::isfinite(f)) throw std::runtime_error("Non-finite material color."); return f; }
    void align() { skip((4 - at % 4) % 4); }
    ModelShaderRecord shader(bool vertex)
    {
        if (u32() == 0) skip(8);
        const auto size0 = u32(), size1 = u32();
        ModelShaderRecord record;
        record.headerSize = vertex ? 0x368 : 0x28;
        if (vertex || size0) {
            record.virtualOffset = at; record.virtualSize = size0;
            skip(size0); align();
            record.physicalOffset = at; record.physicalSize = size1;
            skip(size1); align();
        }
        return record;
    }
};
void TextureHeaders(const CMChunk& chunk, std::vector<CMChunkResourceHeader>& headers)
{
    if (chunk.GetMaskedID() == GenSub_Resource) {
        if (const auto* h = chunk.FindChild(GenSub_ResourceHeader)) {
            CMChunkResourceHeader header(*h);
            if (header.GetResourceType() == kCMResourceType_Texture) headers.push_back(std::move(header));
        }
        return;
    }
    for (const auto& child : chunk.children) TextureHeaders(child, headers);
}
const CMChunk* GeometryResource(const CMChunk& chunk, const CMChunkResourceHeader& header)
{
    if (chunk.GetMaskedID() == GenSub_Resource || chunk.GetMaskedID() == Gen_PostLoadData) {
        if (const auto* h = chunk.FindChild(GenSub_ResourceHeader)) {
            CMChunkResourceHeader candidate(*h);
            if (candidate.GetResourceType() == kCMResourceType_Geometry && candidate.GetCRC() == header.GetCRC() && candidate.GetName() == header.GetName() && candidate.GetDataOffset() == header.GetDataOffset() && candidate.GetLanguageMask() == header.GetLanguageMask() && candidate.GetQualityLevel() == header.GetQualityLevel() && chunk.FindChild(0x321)) return &chunk;
        }
    }
    for (const auto& child : chunk.children) if (const auto* found = GeometryResource(child, header)) return found;
    return nullptr;
}
ModelMaterial Material(const CMChunk& chunk)
{
    Reader r{chunk.data};
    r.skip(244);
    ModelMaterial m;
    m.source = &chunk; m.flags = r.word(32);
    m.name = std::string(reinterpret_cast<const char*>(chunk.data.data()), strnlen(reinterpret_cast<const char*>(chunk.data.data()), 32));
    const auto samplers = r.word(100), stages = r.word(104), slots = r.word(108), sets = r.word(112);
    if (samplers > 12 || stages > 4 || slots > 32 || sets > 8) throw std::runtime_error("Unsupported model material counts.");
    const size_t stageStart = 244 + size_t(samplers) * 140;
    r.skip(size_t(samplers) * 140 + size_t(stages) * 32);
    for (uint32_t i = 0; i < samplers; ++i) {
        const size_t at = 244 + i * 140;
        const char* name = reinterpret_cast<const char*>(chunk.data.data()+at+12);
        m.samplers.push_back({r.word(at), chunk.data[at+4], std::string(name, strnlen(name,128))});
    }
    if (stages) {
        const uint8_t sampler = chunk.data[stageStart + 23];
        m.uvSet = chunk.data[stageStart + 24];
        for (int i = 0; i < 4; ++i) m.color[i] = r.number(stageStart + i * 4);
        if (sampler < samplers) {
            m.textureCRC = r.word(244 + sampler * 140);
            m.addressMode = chunk.data[244 + sampler * 140 + 4];
        }
    }
    for (uint32_t p = 0; p < slots * sets; ++p) {
        const size_t passStart = r.at;
        r.skip(188);
        const uint32_t declSize = r.u32();
        const size_t declStart = r.at;
        r.skip(declSize); r.align();
        if (declSize >= 52) {
            const auto count = r.word(declStart + 24);
            if (count > 32 || count > (declSize - 52) / 12) throw std::runtime_error("Invalid model vertex declaration.");
            ModelDeclaration decl;
            decl.format = r.word(passStart);
            decl.slot = r.word(passStart+180); decl.set = r.word(passStart+184);
            for (uint32_t i = 0; i < count; ++i) {
                const size_t a = declStart + 52 + i * 12;
                const auto streamOffset = r.word(a);
                decl.attributes.push_back({uint16_t(streamOffset >> 16), uint16_t(streamOffset), r.word(a + 4), chunk.data[a+9], chunk.data[a+10]});
            }
            m.declarations.push_back(std::move(decl));
        }
        ModelPass pass;
        pass.format = r.word(passStart); pass.slot = r.word(passStart+180); pass.set = r.word(passStart+184);
        pass.vertex = r.shader(true); pass.pixel = r.shader(false);
        m.passes.push_back(pass);
    }
    return m;
}
void Materials(const CMChunk& chunk, std::vector<ModelMaterial>& out)
{
    if (chunk.GetMaskedID() == 0x331) {
        if (chunk.version != 64) throw std::runtime_error("Unsupported model material version (expected EOT v64).");
        out.push_back(Material(chunk));
        return;
    }
    for (const auto& child : chunk.children) Materials(child, out);
}
}
const CMChunk* FindModelResource(const PKPackage& package, const CMChunkResourceHeader& resource)
{
    for (const auto& root : package.rootChunks) if (const auto* geometry = GeometryResource(root, resource)) return geometry;
    return nullptr;
}
std::vector<ModelMaterial> LoadModelMaterials(const PKPackage& package, const CMChunkResourceHeader& resource)
{
    std::vector<ModelMaterial> result;
    const CMChunk* geometry = FindModelResource(package, resource);
    if (!geometry) return result;
    Materials(*geometry, result);
    std::vector<CMChunkResourceHeader> headers;
    for (const auto& root : package.rootChunks) TextureHeaders(root, headers);
    for (auto& m : result) {
        for (const auto& h : headers) {
            if (h.GetCRC() != m.textureCRC || !m.textureCRC) continue;
            if (!m.texture || (h.GetLanguageMask() == resource.GetLanguageMask() && h.GetQualityLevel() == resource.GetQualityLevel())) m.texture = h;
        }
    }
    return result;
}
const ModelDeclaration* ModelVertexDeclaration(const ModelMaterial& material, uint32_t format)
{
    const uint32_t variantMask = 0xD000;
    for (const auto& d : material.declarations) if (d.format == format) return &d;
    for (const auto& d : material.declarations) if ((d.format & variantMask) == (format & variantMask)) return &d;
    return nullptr;
}

std::vector<uint8_t> ModelShaderContainer(const ModelMaterial& material, const ModelShaderRecord& record)
{
    if (!record.virtualSize) return {};
    if (!material.source || record.virtualSize < record.headerSize) throw std::runtime_error("Invalid shader object image.");
    Reader r{material.source->data};
    r.at = record.virtualOffset; r.skip(record.virtualSize);
    r.at = record.physicalOffset; r.skip(record.physicalSize);
    const auto start = record.virtualOffset + record.headerSize;
    const auto bytes = record.virtualSize - record.headerSize;
    if (bytes < 36 || (r.word(start) & 0xFFFFFFFEu) != 0x102A1100u || r.word(start+4) != bytes || r.word(start+8) != record.physicalSize)
        throw std::runtime_error("Unsupported shader container layout.");
    std::vector<uint8_t> result(material.source->data.begin()+start, material.source->data.begin()+start+bytes);
    result.insert(result.end(), material.source->data.begin()+record.physicalOffset, material.source->data.begin()+record.physicalOffset+record.physicalSize);
    return result;
}
