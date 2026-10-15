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
    const bool collapsed = hudCollapsed && !compact;
    const ImVec2 size(std::min(width - padding * 2, ImGui::GetFontSize() * (collapsed ? 10 : 18)),
        std::min(height - padding * 2, (ImGui::GetFrameHeight() + 3) * (compact || collapsed ? 1 : (materials.empty() ? 3 : 4)) + padding * 2));
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
        if (ImGui::ArrowButton("##Collapse model HUD", collapsed ? ImGuiDir_Right : ImGuiDir_Down)) {
            hudCollapsed = !hudCollapsed;
            if (popup) ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemTooltip(collapsed ? "Expand model controls" : "Condense model controls");
        ImGui::SameLine();
        if (!collapsed) ImGui::TextUnformatted("Model controls");
        if (ImGui::Checkbox("Textures", &textured)) renderDirty = true;
        if (!collapsed) {
            if (ImGui::Checkbox("Two-sided", &twoSided)) renderDirty = true;
            ImGui::SameLine();
            if (ImGui::Button("Frame model")) { ComputeBounds(); renderDirty = true; }
            if (!materials.empty()) {
                if (ImGui::Button("Adjustments...")) ImGui::OpenPopup("Model adjustments");
                if (ImGui::BeginPopup("Model adjustments")) {
                    SelectMaterial("##Adjustment material");
                    if (ImGui::Checkbox("Cutout alpha", &materials[materialIndex].alphaCutout)) renderDirty = true;
                    ImGui::SetItemTooltip("Use the selected material's texture alpha as a cutout mask.");
                    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10);
                    if (ImGui::DragFloat("UV divisor", &uvDivisor, 1, 1, 65536, "%.0f", ImGuiSliderFlags_AlwaysClamp)) renderDirty = true;
                    ImGui::SetItemTooltip("Integer UV coordinates only. OfficeSuit uses 1024; animated UV transforms are not emulated.");
                    ImGui::EndPopup();
                }
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
        ImGui::Text("Retexture: %s", std::filesystem::u8path(texture.png).filename().u8string().c_str());
        ImGui::SetItemTooltip("%s", texture.png.c_str());
    }
    const bool inlineSources = ImGui::GetContentRegionAvail().x >= ImGui::CalcTextSize("Package textureRetexture").x + ImGui::GetFrameHeight() * 2 + ImGui::GetStyle().ItemSpacing.x * 3;
    if (ImGui::RadioButton("Package texture", !texture.useRetexture)) UsePackageTexture(materialIndex);
    if (inlineSources) ImGui::SameLine();
    ImGui::BeginDisabled(!texture.retexture);
    const bool useRetexture = ImGui::RadioButton("Retexture", texture.useRetexture);
    ImGui::EndDisabled();
    if (useRetexture) UseRetexture(materialIndex);
    const ImVec2 buttonSize(-1, 0);
    try {
        ImGui::BeginDisabled(!material.texture);
        const bool extract = ImGui::Button("Extract package PNG...", buttonSize);
        ImGui::EndDisabled();
        if (extract) {
            const auto path = PickFile(glfwGetCurrentContext(), "PNG image", "png", true, SanitizeFileName(material.texture->GetName()) + ".png");
            if (!path.empty()) { ExtractTexture(session, *material.texture, path); Log("Package texture extracted: " + path); }
        }
        if (ImGui::Button("Load retexture PNG...", buttonSize)) {
            const auto path = PickFile(glfwGetCurrentContext(), "PNG image", "png", false);
            if (!path.empty()) LoadPNG(materialIndex, path);
        }
        ImGui::BeginDisabled(texture.png.empty());
        const float halfWidth = std::max(1.f, (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f);
        const bool reload = ImGui::Button("Reload PNG", ImVec2(halfWidth, 0));
        ImGui::SameLine();
        const bool clear = ImGui::Button("Clear", buttonSize);
        ImGui::EndDisabled();
        if (reload) LoadPNG(materialIndex, texture.png);
        if (clear) ClearRetexture(materialIndex);
        ImGui::BeginDisabled(!material.texture || texture.png.empty());
        const bool apply = ImGui::Button("Apply retexture to package", buttonSize);
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
