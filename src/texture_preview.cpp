#include "texture_preview.h"
#include "texture_actions.h"
#include "resource_selection.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <algorithm>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

TexturePreview::~TexturePreview() { Clear(); }

void TexturePreview::Clear()
{
    if (texture)
        glDeleteTextures(1, &texture);
    texture = 0;
    selected = nullptr;
    package = nullptr;
    active = false;
    width = height = 0;
    name.clear();
    error.clear();
}

void TexturePreview::Update(const PackageSession& session)
{
    if (selected == session.selected && package == session.package.get() && revision == session.revision)
        return;
    Clear();
    selected = session.selected;
    package = session.package.get();
    revision = session.revision;
    if (!selected || !package)
        return;
    try
    {
        const auto resource = SelectedResource(*package, selected);
        if (!resource || resource->GetResourceType() != kCMResourceType_Texture)
            return;
        active = true;
        name = resource->GetName();
        const RZTexture decoded = SelectedTexture(session);
        descriptor = decoded.desc;
        width = decoded.desc.width;
        height = decoded.desc.height;
        GLint maximum = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximum);
        if (!width || !height || width > static_cast<unsigned int>(maximum) || height > static_cast<unsigned int>(maximum))
            throw std::runtime_error("Texture dimensions exceed the renderer's limits.");
        const auto rgba = decoded.DecodeLevel0();
        if (rgba.size() != static_cast<size_t>(width) * height * 4)
            throw std::runtime_error("Texture decoder returned an unexpected pixel count.");
        GLint previous = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
        glBindTexture(GL_TEXTURE_2D, previous);
        if (glGetError() != GL_NO_ERROR)
            throw std::runtime_error("Failed to upload texture preview.");
    }
    catch (const std::exception& ex)
    {
        active = true;
        error = ex.what();
    }
}

void TexturePreview::Draw()
{
    ImGui::TextWrapped("%s", name.c_str());
    if (!error.empty())
    {
        ImGui::TextWrapped("Preview unavailable: %s", error.c_str());
        return;
    }
    ImGui::TextDisabled("%u x %u", width, height);
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float scale = std::min({1.0f, available.x / width, available.y / height});
    if (scale <= 0)
        return;
    const ImVec2 size(width * scale, height * scale);
    const ImVec2 cursor = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(cursor.x + (available.x - size.x) / 2,
        cursor.y + (available.y - size.y) / 2));
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    constexpr float tile = 16;
    for (int row = 0; row * tile < size.y; ++row)
        for (int column = 0; column * tile < size.x; ++column)
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(origin.x + column * tile, origin.y + row * tile),
                ImVec2(origin.x + std::min((column + 1) * tile, size.x),
                    origin.y + std::min((row + 1) * tile, size.y)),
                (row + column) % 2 ? IM_COL32(215, 215, 215, 255) : IM_COL32(240, 240, 240, 255));
    ImGui::Image(static_cast<ImTextureID>(texture), size);
}
