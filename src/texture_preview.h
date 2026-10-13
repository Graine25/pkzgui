#pragma once
#include "package_session.h"
#include "Resource/RZTexture.h"

class TexturePreview
{
public:
    TexturePreview() = default;
    TexturePreview(const TexturePreview&) = delete;
    TexturePreview& operator=(const TexturePreview&) = delete;
    ~TexturePreview();
    void Update(const PackageSession& session);
    void Draw();
    void Clear();

    bool active = false;
    unsigned int width = 0, height = 0;
    std::string name, error;
    RZTexture::Descriptor descriptor;

private:
    const CMChunk* selected = nullptr;
    const PKPackage* package = nullptr;
    unsigned int texture = 0;
    uint64_t revision = 0;
};
