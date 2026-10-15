#include "texture/texture_actions.h"
#include "package/resource_selection.h"
#include "platform/file_io.h"
#include "BaseUtils/BUPng.h"

RZTexture SelectedTexture(const PackageSession& session)
{
    if (!session.package)
        throw std::runtime_error("Select a texture first.");
    const auto header = SelectedResource(*session.package, session.selected);
    if (!header || header->GetResourceType() != kCMResourceType_Texture)
        throw std::runtime_error("Select a texture first.");
    return RZTexture::Load(*session.package, *header);
}

void ExtractTexture(const PackageSession& session, const std::string& path)
{
    const auto header = session.package ? SelectedResource(*session.package, session.selected) : std::nullopt;
    if (!header || header->GetResourceType() != kCMResourceType_Texture)
        throw std::runtime_error("Select a texture first.");
    ExtractTexture(session, *header, path);
}

void ExtractTexture(const PackageSession& session, const CMChunkResourceHeader& header, const std::string& path)
{
    if (!session.package || header.GetResourceType() != kCMResourceType_Texture) throw std::runtime_error("Select a texture first.");
    const RZTexture texture = RZTexture::Load(*session.package, header);
    const auto rgba = texture.DecodeLevel0();
    WriteFile(std::filesystem::u8path(path), BUPng::EncodeRGBA8(rgba.data(), texture.desc.width, texture.desc.height));
}

void ReplaceTexture(PackageSession& session, const std::string& path, uint32_t width, uint32_t height)
{
    const auto header = session.package ? SelectedResource(*session.package, session.selected) : std::nullopt;
    if (!header || header->GetResourceType() != kCMResourceType_Texture)
        throw std::runtime_error("Select a texture first.");
    ReplaceTexture(session, *header, path, width, height);
}

void ReplaceTexture(PackageSession& session, const CMChunkResourceHeader& header, const std::string& path, uint32_t width, uint32_t height)
{
    if (!width || !height || width > 4096 || height > 4096)
        throw std::runtime_error("Width/height must be between 1 and 4096.");
    const BUPng::Image image = BUPng::Decode(ReadFile(std::filesystem::u8path(path)));
    if (!session.package || header.GetResourceType() != kCMResourceType_Texture) throw std::runtime_error("Select a texture first.");
    RZTexture texture = RZTexture::Load(*session.package, header);
    std::vector<uint8_t> rgba(static_cast<size_t>(width) * height * 4);
    for (uint32_t y = 0; y < height; ++y)
        for (uint32_t x = 0; x < width; ++x)
        {
            const size_t source = (static_cast<size_t>(y) * image.height / height * image.width +
                static_cast<size_t>(x) * image.width / width) * 4;
            std::copy_n(image.rgba.data() + source, 4, rgba.data() + (static_cast<size_t>(y) * width + x) * 4);
        }
    texture.Reencode(rgba.data(), width, height);
    texture.Store(*session.package);
    for (CMChunk& root : session.package->rootChunks)
        root.ComputeLength();
    session.dirty = true;
    ++session.revision;
    session.status = "Texture replaced (original format kept). Save the package to write to disk.";
}
