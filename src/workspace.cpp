#include "workspace.h"
#include "imgui.h"
#include <algorithm>
#include <cstdio>

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
        const std::string label = ToString(chunk.GetIDToEnum()) + " [" + Hex(chunk.GetMaskedID()) +
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
        ImGui::TextUnformatted(value.c_str());
    }
}

void Workspace::Draw()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Workspace", nullptr, ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

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
    ImGui::BeginChild("Fields", ImVec2(0, bodyHeight), ImGuiChildFlags_Borders);
    if (ImGui::BeginTable("Field table", 4, ImGuiTableFlags_Resizable |
        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoBordersInBody |
        ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingFixedFit, ImVec2(0, 0)))
    {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 260);
        ImGui::TableSetupColumn("Offset", ImGuiTableColumnFlags_WidthFixed, 90);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 140);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 480);
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
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();

    ImGui::BeginChild("Status", ImVec2(0, statusHeight), ImGuiChildFlags_Borders);
    ImGui::TextUnformatted(session.status.c_str());
    ImGui::EndChild();
    ImGui::End();
    ImGui::PopStyleVar();

    if (!session.error.empty())
        ImGui::OpenPopup("Failed to open package");
    if (ImGui::BeginPopupModal("Failed to open package", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
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
