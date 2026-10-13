#include "workspace.h"
#include "imgui.h"
#include <algorithm>

bool Workspace::Draw()
{
    bool exit = false;
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("Open package...", "Ctrl+O", false, false);
            ImGui::MenuItem("Save", "Ctrl+S", false, false);
            ImGui::MenuItem("Save As...", nullptr, false, false);
            ImGui::MenuItem("Load schema...", nullptr, false, false);
            ImGui::Separator();
            exit = ImGui::MenuItem("Exit");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            ImGui::MenuItem("Edit selected field...", "Enter", false, false);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Expand all", nullptr, false, false);
            ImGui::MenuItem("Collapse all", nullptr, false, false);
            ImGui::Separator();
            ImGui::MenuItem("Simple view", nullptr, false, false);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Texture"))
        {
            ImGui::MenuItem("Extract texture (PNG)...", nullptr, false, false);
            ImGui::MenuItem("Replace texture (PNG)...", nullptr, false, false);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Audio"))
        {
            ImGui::MenuItem("Extract sample (WAV)...", nullptr, false, false);
            ImGui::MenuItem("Play sample", nullptr, false, false);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

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
        ImGui::EndTable();
    }
    ImGui::EndChild();

    ImGui::BeginChild("Status", ImVec2(0, statusHeight), ImGuiChildFlags_Borders);
    ImGui::TextUnformatted("No package loaded.");
    ImGui::EndChild();
    ImGui::End();
    ImGui::PopStyleVar();
    return exit;
}
