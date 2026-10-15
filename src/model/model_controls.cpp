#include "model/model_preview.h"
#include "platform/file_dialog.h"
#include "platform/file_io.h"
#include "texture/texture_actions.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <utility>

void ModelPreview::Log(const std::string& message)
{
    logMessages.push_back(message);
}

std::vector<std::string> ModelPreview::TakeLogMessages()
{
    auto messages = std::move(logMessages);
    logMessages.clear();
    return messages;
}

void ModelPreview::SelectMaterial(const char* id)
{
    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo(id, materials[materialIndex].name.c_str())) {
        for (size_t i = 0; i < materials.size(); ++i)
            if (ImGui::Selectable(materials[i].name.c_str(), materialIndex == int(i))) materialIndex = int(i);
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("Material");
}

bool ModelPreview::DrawHUD(float x, float y, float width, float height)
{
    if (width < 64 || height < 48) return false;
    const float padding = 8;
    const bool compact = width < ImGui::GetFontSize() * 16 || height < ImGui::GetFrameHeightWithSpacing() * 4;
    const ImVec2 size(std::min(width - padding * 2, ImGui::GetFontSize() * 18),
        std::min(height - padding * 2, (ImGui::GetFrameHeight() + 3) * (compact ? 1 : (materials.empty() ? 3 : 5)) + padding * 2));
    const ImVec2 pos(x + width - size.x - padding, y + height - size.y - padding);
    ImGui::SetCursorScreenPos(pos);
    ImVec4 background = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    background.w = 0.95f;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, background);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 3));
    ImGui::BeginChild("Model HUD", size, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    if (compact && ImGui::Button("View...", ImVec2(-1, 0))) ImGui::OpenPopup("Model view controls");
    if (compact) ImGui::SetNextWindowSizeConstraints(ImVec2(ImGui::GetFontSize() * 18, 0), ImGui::GetMainViewport()->WorkSize);
    const bool popup = compact && ImGui::BeginPopup("Model view controls");
    if (!compact || popup) {
        ImGui::TextUnformatted("Model controls");
        ImGui::SetItemTooltip("Drag the model to orbit; scroll to zoom.");
        if (!materials.empty()) SelectMaterial("##View material");
        if (ImGui::Checkbox("Package textures", &textured)) renderDirty = true;
        if (ImGui::Checkbox("Two-sided", &twoSided)) renderDirty = true;
        ImGui::SameLine();
        if (ImGui::Button("Frame model")) { ComputeBounds(); renderDirty = true; }
        if (!materials.empty()) {
            if (ImGui::Button("Adjustments...")) ImGui::OpenPopup("Model adjustments");
            if (ImGui::BeginPopup("Model adjustments")) {
                if (ImGui::Checkbox("Cutout alpha", &materials[materialIndex].alphaCutout)) renderDirty = true;
                ImGui::SetItemTooltip("Use the selected material's texture alpha as a cutout mask.");
                ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10);
                if (ImGui::DragFloat("UV divisor", &uvDivisor, 1, 1, 65536, "%.0f", ImGuiSliderFlags_AlwaysClamp)) renderDirty = true;
                ImGui::SetItemTooltip("Integer UV coordinates only. OfficeSuit uses 1024; animated UV transforms are not emulated.");
                ImGui::EndPopup();
            }
        }
    }
    if (popup) ImGui::EndPopup();
    const bool blocksInput = ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + size.x, pos.y + size.y)) || ImGui::IsPopupOpen("Model adjustments") || ImGui::IsPopupOpen("Model view controls");
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    return blocksInput;
}

void ModelPreview::DrawActions(PackageSession& session)
{
    ImGui::TextUnformatted("Model exports / textures");
    ImGui::Separator();
    if (materials.empty() || !error.empty()) {
        ImGui::TextDisabled("No model materials available.");
        return;
    }
    SelectMaterial("##Export material");
    const auto& material = materials[materialIndex];
    const auto& texture = materialTextures[materialIndex];
    if (material.texture) {
        ImGui::TextUnformatted(material.texture->GetName().c_str());
        ImGui::SetItemTooltip("Package texture: %s", material.texture->GetName().c_str());
    } else {
        ImGui::TextDisabled("No package texture");
        ImGui::SetItemTooltip(material.textureCRC ? "The referenced texture is not in this package. A PNG can still be previewed." : "This material has no base-color texture.");
    }
    if (!texture.png.empty()) {
        ImGui::Text("Preview: %s", std::filesystem::u8path(texture.png).filename().u8string().c_str());
        ImGui::SetItemTooltip("%s", texture.png.c_str());
    }
    const ImVec2 buttonSize(-1, 0);
    try {
        ImGui::BeginDisabled(!material.texture);
        const bool extract = ImGui::Button("Extract package PNG...", buttonSize);
        ImGui::EndDisabled();
        if (extract) {
            const auto path = PickFile(glfwGetCurrentContext(), "PNG image", "png", true, SanitizeFileName(material.texture->GetName()) + ".png");
            if (!path.empty()) { ExtractTexture(session, *material.texture, path); Log("Package texture extracted: " + path); }
        }
        if (ImGui::Button("Preview PNG...", buttonSize)) {
            const auto path = PickFile(glfwGetCurrentContext(), "PNG image", "png", false);
            if (!path.empty()) LoadPNG(materialIndex, path);
        }
        ImGui::BeginDisabled(texture.png.empty());
        const bool restore = ImGui::Button("Use package texture", buttonSize);
        ImGui::EndDisabled();
        if (restore) UsePackageTexture(materialIndex);
        ImGui::BeginDisabled(!material.texture || texture.png.empty());
        const bool apply = ImGui::Button("Apply PNG to package", buttonSize);
        ImGui::SetItemTooltip("Replaces every use of this texture. Save As writes the edited package.");
        ImGui::EndDisabled();
        if (apply) ApplyPNG(session, materialIndex);
        ImGui::Separator();
        ImGui::BeginDisabled(material.passes.empty());
        const bool exportShaders = ImGui::Button("Export material shaders...", buttonSize);
        ImGui::EndDisabled();
        if (exportShaders) {
            const auto path = PickFile(glfwGetCurrentContext(), "Shader containers", "bin", true, SanitizeFileName(material.name) + ".bin");
            if (!path.empty()) { ExportModelShaders(material, path); Log("Shader containers and binding metadata exported for XenosRecomp: " + path); }
        }
    } catch (const std::exception& ex) { Log("Model texture/export error: " + std::string(ex.what())); }
}
