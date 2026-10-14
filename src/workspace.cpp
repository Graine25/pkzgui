#include "workspace.h"
#include "imgui.h"
#include <algorithm>
#include <cstdio>
#include "texture_actions.h"

namespace
{
    std::string Hex(uint64_t value)
    {
        char text[32];
        std::snprintf(text, sizeof(text), "0x%llX", static_cast<unsigned long long>(value));
        return text;
    }

    void DrawChunk(const CMChunk& chunk, PackageSession& session, bool root = false)
    {
        std::string title = ToString(chunk.GetIDToEnum());
        if (chunk.GetMaskedID() == GenSub_Resource)
            if (const CMChunk* header = chunk.FindChild(GenSub_ResourceHeader))
                try { title = "\"" + CMChunkResourceHeader(*header).GetName() + "\""; } catch (const std::exception&) {}
        const std::string label = title + " [" + Hex(chunk.GetMaskedID()) +
            "] v" + std::to_string(chunk.version) + (chunk.GetHasChildren() ? " folder " : " leaf ") +
            std::to_string(chunk.length);
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (chunk.children.empty())
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (root)
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        if (&chunk == session.selected)
            flags |= ImGuiTreeNodeFlags_Selected;
        const bool open = ImGui::TreeNodeEx(&chunk, flags, "%s", label.c_str());
        if (ImGui::IsItemClicked())
            session.selected = &chunk;
        if (open && !chunk.children.empty())
        {
            for (const CMChunk& child : chunk.children)
                DrawChunk(child, session);
            ImGui::TreePop();
        }
    }

    void DrawRow(const char* name, const char* type, const std::string& value)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(name);
        ImGui::TableSetColumnIndex(2);
        ImGui::TextUnformatted(type);
        ImGui::TableSetColumnIndex(3);
        ImGui::PushTextWrapPos(0);
        ImGui::TextUnformatted(value.c_str());
        ImGui::PopTextWrapPos();
    }
}

void Workspace::Draw()
{
    session.PollLoad();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Workspace", nullptr, ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    ImGui::BeginDisabled(IsBusy());

    const float statusHeight = ImGui::GetFrameHeight();
    const float bodyHeight = std::max(1.0f, ImGui::GetContentRegionAvail().y - statusHeight);
    const float width = ImGui::GetContentRegionAvail().x;
    constexpr float gap = 4.0f;
    splitX = std::clamp(splitX, std::min(120.0f, width * 0.4f), std::max(120.0f, width - 160.0f - gap));
    ImGui::BeginChild("Chunk tree", ImVec2(splitX, bodyHeight), ImGuiChildFlags_Borders);
    if (session.package)
        for (const CMChunk& chunk : session.package->rootChunks)
            DrawChunk(chunk, session, true);
    ImGui::EndChild();
    ImGui::SameLine(0, 0);
    ImGui::InvisibleButton("Splitter", ImVec2(gap, bodyHeight));
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    if (ImGui::IsItemActive())
        splitX += ImGui::GetIO().MouseDelta.x;
    ImGui::SameLine(0, 0);
    RefreshSelection();
    preview.Update(session);
    ImGui::BeginChild("Right pane", ImVec2(0, bodyHeight));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
    const float fieldsHeight = preview.active ? bodyHeight * 0.4f : 0;
    ImGui::BeginChild("Fields", ImVec2(0, fieldsHeight),
        ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::TextUnformatted(session.selected ? ToString(session.selected->GetIDToEnum()).c_str() : "Chunk details");
    ImGui::Spacing();
    if (ImGui::BeginTable("Field table", 4, ImGuiTableFlags_Resizable |
        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoBordersInBody |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp, ImVec2(0, 0)))
    {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.30f);
        ImGui::TableSetupColumn("Offset", ImGuiTableColumnFlags_WidthStretch, 0.12f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 0.16f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.42f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        if (session.selected)
        {
            const CMChunk& chunk = *session.selected;
            DrawRow("Chunk", "", ToString(chunk.GetIDToEnum()));
            DrawRow("ID", "u32", Hex(chunk.GetMaskedID()));
            DrawRow("Version", "u16", std::to_string(chunk.version));
            DrawRow("Length", "", std::to_string(chunk.length));
            DrawRow("Offset", "", Hex(chunk.offset));
            DrawRow("Header Size", "", std::to_string(chunk.HeaderSize()));
            DrawRow("Children", "", std::to_string(chunk.children.size()));
            if (resource)
            {
                DrawRow("name", "string", resource->GetName());
                DrawRow("type", "", resource->GetResourceTypeName());
                DrawRow("crc", "u32", Hex(resource->GetCRC()));
                DrawRow("languageMask", "u32", Hex(resource->GetLanguageMask()));
                DrawRow("qualityLevel", "u32", std::to_string(resource->GetQualityLevel()));
                DrawRow("dataOffset", "u64", Hex(resource->GetDataOffset()));
                DrawRow("postLoadDataCRC", "u32", Hex(resource->GetPostLoadDataCRC()));
            }
            if (preview.active && preview.error.empty())
            {
                DrawRow("Texture", "string", preview.name);
                DrawRow("Width", "u32", std::to_string(preview.width));
                DrawRow("Height", "u32", std::to_string(preview.height));
                DrawRow("dimension", "u32", std::to_string(preview.descriptor.dimension));
                DrawRow("mipLevels", "u32", std::to_string(preview.descriptor.mipLevels));
                DrawRow("depth", "u32", std::to_string(preview.descriptor.depth));
                DrawRow("d3dFormat", "u32", Hex(preview.descriptor.d3dFormat));
            }

        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
    if (preview.active)
    {
        ImGui::Dummy(ImVec2(0, gap));
        ImGui::BeginChild("Texture preview", ImVec2(0, 0),
            ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
        preview.Draw();
        ImGui::EndChild();
    }

    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::BeginChild("Status", ImVec2(0, statusHeight), ImGuiChildFlags_Borders);
    ImGui::TextUnformatted(session.IsLoading() ? "Loading package..." : session.status.c_str());
    ImGui::EndChild();
    ImGui::EndDisabled();
    ImGui::End();
    ImGui::PopStyleVar();

    if (IsBusy())
    {
        ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted("Loading package...");
        ImGui::End();
    }

    if (!replacementPath.empty()) ImGui::OpenPopup("Replace texture");
    if (ImGui::BeginPopupModal("Replace texture", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputInt("Width", &replacementWidth);
        ImGui::InputInt("Height", &replacementHeight);
        ImGui::TextUnformatted("Encoded in the original GPU format only.");
        if (ImGui::Button("OK"))
        {
            try { ReplaceTexture(session, replacementPath, replacementWidth, replacementHeight); }
            catch (const std::exception& ex) { session.error = ex.what(); }
            replacementPath.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) { replacementPath.clear(); ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }

    if (confirmOpen) ImGui::OpenPopup("Open package");
    if (ImGui::BeginPopupModal("Open package", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("Discard unsaved changes?");
        if (ImGui::Button("Yes")) { approvedOpen = true; confirmOpen = false; ImGui::CloseCurrentPopup(); }
        ImGui::SameLine();
        if (ImGui::Button("No")) { confirmOpen = false; ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }

    if (!session.error.empty())
        ImGui::OpenPopup("Operation failed");
    if (ImGui::BeginPopupModal("Operation failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 40);
        ImGui::TextUnformatted(session.error.c_str());
        ImGui::PopTextWrapPos();
        if (ImGui::Button("Close"))
        {
            session.error.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
