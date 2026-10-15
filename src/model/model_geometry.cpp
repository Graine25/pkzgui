#include "model/model_geometry.h"
#include "Resource/RZTexture.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace {
    uint16_t ReadBE16(const uint8_t* p)
    {
        return static_cast<uint16_t>((p[0] << 8) | p[1]);
    }

    uint32_t ReadBE32(const uint8_t* p)
    {
        return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
            (static_cast<uint32_t>(p[2]) << 8) | p[3];
    }

    float ReadBEf32(const uint8_t* p)
    {
        uint32_t u = ReadBE32(p);
        float f;
        std::memcpy(&f, &u, 4);
        return f;
    }

    int s11(int v) { return v > 1023 ? v - 2048 : v; }
    int s10(int v) { return v > 511 ? v - 1024 : v; }

    void UnpackNormal(uint32_t u, float& x, float& y, float& z)
    {
        x = s11(static_cast<int>(u & 0x7FF)) / 1024.0f;
        y = s11(static_cast<int>((u >> 11) & 0x7FF)) / 1024.0f;
        z = s10(static_cast<int>((u >> 22) & 0x3FF)) / 512.0f;
    }

    struct Desc
    {
        uint32_t mat = 0, vboff = 0, stride = 0, fmt = 0, nv = 0, idxoff = 0, nidx = 0;
    };

    int Nuv(uint32_t fmt) { return (fmt >> 5) & 15; }

    struct Offsets
    {
        int nrm = -1, uv = -1;
    };

    Offsets AttrOffsets(uint32_t fmt)
    {
        Offsets o;
        int p = 12;
        if (fmt & 0x0002) { o.nrm = p; p += 4; }
        if (fmt & 0x0004) p += 4;
        if (fmt & 0x0008) p += 4;
        if (fmt & 0x0010) p += 4;
        if (Nuv(fmt)) { o.uv = p; p += 4 * Nuv(fmt); }
        return o;
    }

    std::vector<uint32_t> StripToTris(const uint16_t* idx, size_t n, uint32_t nv)
    {
        std::vector<uint32_t> tris;
        std::vector<uint16_t> run;
        auto flush = [&]() {
            for (size_t i = 0; i + 2 < run.size(); ++i)
            {
                const uint16_t a = run[i], b = run[i + 1], c = run[i + 2];
                if (a == b || b == c || a == c) continue;
                if (a >= nv || b >= nv || c >= nv) continue;
                if (i % 2 == 0) { tris.push_back(b); tris.push_back(a); tris.push_back(c); }
                else { tris.push_back(a); tris.push_back(b); tris.push_back(c); }
            }
            run.clear();
            };
        for (size_t i = 0; i < n; ++i)
        {
            const uint16_t x = idx[i];
            if (x == 0xFFFF)
            {
                flush();
                continue;
            }
            if (x >= nv) throw std::runtime_error("Mesh index exceeds the vertex count.");
            run.push_back(x);
        }
        flush();
        return tris;
    }

    bool ParseDescriptor(const uint8_t* data, size_t size, Desc& d)
    {
        if (size < 48) return false;
        d.mat = ReadBE32(data + 0);
        d.vboff = ReadBE32(data + 4);
        d.stride = ReadBE32(data + 8);
        d.fmt = ReadBE32(data + 12);
        d.nv = ReadBE32(data + 16);
        d.idxoff = ReadBE32(data + 20);
        d.nidx = ReadBE32(data + 24);
        if (d.nv == 0 || d.nidx == 0 || d.stride == 0) return false;
        return true;
    }

    bool FinitePos(float x, float y, float z)
    {
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return false;
        constexpr float lim = 1.0e5f;
        return std::fabs(x) < lim && std::fabs(y) < lim && std::fabs(z) < lim;
    }

    void DecodeMesh(const uint8_t* vb, size_t vbSize, const uint8_t* ib, size_t ibSize,
        const Desc& d, ModelMesh& out, const std::vector<ModelMaterial>& materials)
    {
        out.material = d.mat;
        out.positions.clear();
        out.normals.clear();
        out.indices.clear();

        const uint32_t stride = d.stride;
        if (stride < 12 || d.nv < 3 || d.nidx < 3) return;
        if (d.nv > 1000000 || d.nidx > 6000000)
            throw std::runtime_error("Mesh exceeds the preview limit (1M vertices / 6M indices).");
        if (d.vboff + static_cast<size_t>(d.nv) * stride > vbSize) return;
        if (static_cast<size_t>(d.idxoff) * 2 + static_cast<size_t>(d.nidx) * 2 > ibSize) return;

        Offsets o = AttrOffsets(d.fmt);
        int positionOffset = 0;
        uint32_t normalType = 16, uvType = 25 | 0x300;
        const ModelMaterial* material = d.mat < materials.size() ? &materials[d.mat] : nullptr;
        if (material) {
            const auto* decl = ModelVertexDeclaration(*material, d.fmt);
            if (!decl) throw std::runtime_error("No matching vertex declaration for " + material->name);
            bool position = false;
            o.nrm = o.uv = -1;
            for (const auto& a : decl->attributes) {
                if (a.stream != 0) continue;
                const uint32_t type = a.type & 63;
                const size_t bytes = type == 57 ? 12 : (type == 37 ? 8 : (type == 38 ? 16 : 4));
                if (a.offset > stride || bytes > stride - a.offset) throw std::runtime_error("Vertex declaration exceeds mesh stride.");
                if (a.usage == 0 && a.index == 0) {
                    if (type != 57) throw std::runtime_error("Unsupported packed position format.");
                    positionOffset = a.offset; position = true;
                }
                if (a.usage == 3 && a.index == 0) { o.nrm = a.offset; normalType = a.type; }
                if (a.usage == 5 && a.index == material->uvSet) { o.uv = a.offset; uvType = a.type; }
            }
            if (!position) throw std::runtime_error("Vertex declaration has no base position stream.");
        }
        out.hasUV = o.uv >= 0;
        out.integerUV = (uvType & 63) == 25 && (uvType & 0x200);
        out.texcoords.reserve(size_t(d.nv) * 2);
        out.positions.reserve(static_cast<size_t>(d.nv) * 3);
        out.normals.reserve(static_cast<size_t>(d.nv) * 3);

        int bad = 0;
        for (uint32_t v = 0; v < d.nv; ++v)
        {
            const uint8_t* p = vb + d.vboff + static_cast<size_t>(v) * stride;
            const float x = ReadBEf32(p + positionOffset);
            const float y = ReadBEf32(p + positionOffset + 4);
            const float z = ReadBEf32(p + positionOffset + 8);
            if (!FinitePos(x, y, z)) ++bad;
            out.positions.push_back(x);
            out.positions.push_back(y);
            out.positions.push_back(z);
            float u = 0, t = 0;
            if (out.hasUV) {
                const auto* uv = p + o.uv;
                if ((uvType & 63) == 25) {
                    const bool signedValue = (uvType & 0x100) != 0;
                    u = signedValue ? float(static_cast<int16_t>(ReadBE16(uv))) : float(ReadBE16(uv));
                    t = signedValue ? float(static_cast<int16_t>(ReadBE16(uv+2))) : float(ReadBE16(uv+2));
                    if (!(uvType & 0x200)) {
                        const float divisor = signedValue ? 32767.f : 65535.f;
                        u = std::max(-1.f, u / divisor); t = std::max(-1.f, t / divisor);
                    }
                } else if ((uvType & 63) == 37) {
                    u = ReadBEf32(uv); t = ReadBEf32(uv + 4);
                } else throw std::runtime_error("Unsupported UV vertex format.");
                if (!std::isfinite(u) || !std::isfinite(t)) throw std::runtime_error("Non-finite model UV.");
            }
            out.texcoords.push_back(u); out.texcoords.push_back(t);
            if (o.nrm >= 0)
            {
                float nx, ny, nz;
                if ((normalType & 63) == 16) UnpackNormal(ReadBE32(p + o.nrm), nx, ny, nz);
                else if ((normalType & 63) == 57) { nx = ReadBEf32(p + o.nrm); ny = ReadBEf32(p + o.nrm + 4); nz = ReadBEf32(p + o.nrm + 8); }
                else throw std::runtime_error("Unsupported normal vertex format.");
                if (!std::isfinite(nx) || !std::isfinite(ny) || !std::isfinite(nz)) throw std::runtime_error("Non-finite model normal.");
                out.normals.push_back(nx);
                out.normals.push_back(ny);
                out.normals.push_back(nz);
            }
            else
            {
                out.normals.push_back(0.f);
                out.normals.push_back(1.f);
                out.normals.push_back(0.f);
            }
        }
        if (bad != 0)
        {
            out.positions.clear();
            out.normals.clear();
            return;
        }

        uint32_t nidx = d.nidx;
        while (nidx > 0)
        {
            const uint16_t last = ReadBE16(ib + (static_cast<size_t>(d.idxoff) + nidx - 1) * 2);
            if (last != 0xAAAA || d.nv > 0xAAAA) break;
            --nidx;
        }
        if (nidx < 3) return;

        std::vector<uint16_t> raw(nidx);
        for (uint32_t i = 0; i < nidx; ++i)
            raw[i] = ReadBE16(ib + (static_cast<size_t>(d.idxoff) + i) * 2);

        out.indices = StripToTris(raw.data(), raw.size(), d.nv);
    }


const CMChunk* InlineGeometry(const CMChunk& chunk, const CMChunkResourceHeader& resource)
{
    if (const auto* h = chunk.FindChild(GenSub_ResourceHeader)) {
        const CMChunkResourceHeader candidate(*h);
        if (candidate.GetResourceType() == kCMResourceType_Geometry && candidate.GetCRC() == resource.GetCRC() &&
            candidate.GetName() == resource.GetName() && candidate.GetDataOffset() == resource.GetDataOffset() &&
            candidate.GetLanguageMask() == resource.GetLanguageMask() && candidate.GetQualityLevel() == resource.GetQualityLevel()) {
            for (const auto& lod : chunk.children)
                if (lod.GetMaskedID() == 0x325 && lod.FindChild(0x1388) && lod.FindChild(0x1389) && lod.FindChild(0x327)) return &chunk;
        }
    }
    for (const auto& child : chunk.children) if (const auto* found = InlineGeometry(child, resource)) return found;
    return nullptr;
}
uint32_t LodIndex(const CMChunk& lod)
{
    const auto* info = lod.FindChild(0x326);
    if (!info || info->data.size() < 16) throw std::runtime_error("Missing geometry LOD information.");
    return ReadBE32(info->data.data() + 12);
}
}

std::vector<ModelMesh> LoadModelGeometry(const PKPackage& package, const CMChunkResourceHeader& resource,
    const std::vector<ModelMaterial>& materials)
{
    const CMChunk* source = FindModelResource(package, resource);
    const CMChunk* post = nullptr;
    if (source && source->FindChild(0x321)->FindChild(0x325)) {
        source = source->FindChild(0x321);
        post = RZTexture::FindRootAtOffset(package, resource.GetDataOffset());
        if (!post || post->GetMaskedID() != Gen_PostLoadData) throw std::runtime_error("Geometry post-load data is missing at the resource's data offset.");
        const auto* h = post->FindChild(GenSub_ResourceHeader);
        if (!h || CMChunkResourceHeader(*h).GetCRC() != resource.GetCRC() || CMChunkResourceHeader(*h).GetResourceType() != kCMResourceType_Geometry)
            throw std::runtime_error("Geometry post-load header does not match the selected resource.");
    } else {
        for (const auto& root : package.rootChunks) if ((source = InlineGeometry(root, resource))) break;
        post = source;
    }
    if (!source || !post) throw std::runtime_error("Geometry resource or its buffers are missing.");
    const CMChunk* descriptors = nullptr;
    uint32_t level = std::numeric_limits<uint32_t>::max();
    for (const auto& lod : source->children) {
        if (lod.GetMaskedID() != 0x325 || !lod.FindChild(0x327)) continue;
        const auto index = source == post ? 0 : LodIndex(lod);
        if (index < level) { level = index; descriptors = &lod; }
    }
    if (!descriptors) throw std::runtime_error("Geometry has no mesh descriptors.");
    const CMChunk* buffers = nullptr;
    for (const auto& lod : post->children) {
        if (lod.GetMaskedID() != 0x325) continue;
        if (source == post || LodIndex(lod) == level) {
            if (buffers) throw std::runtime_error("Duplicate geometry LOD buffers.");
            buffers = &lod;
        }
    }
    const auto* vb = buffers ? buffers->FindChild(0x1388) : nullptr;
    const auto* ib = buffers ? buffers->FindChild(0x1389) : nullptr;
    if (!vb || !ib) throw std::runtime_error("Vertex/index buffers are missing for this LOD.");
    std::vector<ModelMesh> meshes;
    size_t bytes = 0;
    for (const auto& chunk : descriptors->children) {
        if (chunk.GetMaskedID() != 0x327) continue;
        if (source != post && chunk.version != 8) throw std::runtime_error("Unsupported mesh version (expected EOT v8).");
        Desc desc;
        if (!ParseDescriptor(chunk.data.data(), chunk.data.size(), desc)) throw std::runtime_error("Invalid mesh descriptor.");
        if (!materials.empty() && desc.mat >= materials.size()) throw std::runtime_error("Mesh material index is out of range.");
        ModelMesh mesh;
        DecodeMesh(vb->data.data(), vb->data.size(), ib->data.data(), ib->data.size(), desc, mesh, materials);
        if (mesh.positions.empty() || mesh.indices.empty()) throw std::runtime_error("Mesh has no valid triangles or its buffers are truncated.");
        bytes += (mesh.positions.size() + mesh.normals.size() + mesh.texcoords.size()) * sizeof(float) + mesh.indices.size() * sizeof(uint32_t);
        if (bytes > 128 * 1024 * 1024) throw std::runtime_error("Model exceeds the 128 MiB preview budget.");
        meshes.push_back(std::move(mesh));
    }
    return meshes;
}
