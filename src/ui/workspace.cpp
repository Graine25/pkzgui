#include "ui/workspace.h"
#include "imgui.h"
#include <algorithm>
#include <cstdio>
#include "texture/texture_actions.h"
#include <GLFW/glfw3.h>

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
        bool namedResource = false;
        if (chunk.GetMaskedID() == GenSub_Resource)
            if (const CMChunk* header = chunk.FindChild(GenSub_ResourceHeader))
                try {
                const auto name = CMChunkResourceHeader(*header).GetName();
                if (!name.empty()) { title = name; namedResource = true; }
            } catch (const std::exception&) {}
            const std::string label = namedResource ? title : title + " [" + Hex(chunk.GetMaskedID()) +
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
        ImGui::SetItemTooltip("%s", name);
        ImGui::TableSetColumnIndex(2);
        ImGui::TextUnformatted(type);
        ImGui::TableSetColumnIndex(3);
        ImGui::TextUnformatted(value.c_str());
        ImGui::SetItemTooltip("%s", value.c_str());
    }
}

void Workspace::DrawFields()
{
    ImGui::TextUnformatted("Fields");
    if (resource) {
        ImGui::TextUnformatted(resource->GetName().c_str());
        ImGui::SetItemTooltip("%s", resource->GetName().c_str());
    }
    else if (session.selected) ImGui::TextUnformatted(ToString(session.selected->GetIDToEnum()).c_str());
    ImGui::Separator();
    if (ImGui::BeginTable("Field table##single-line", 4,
        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoBordersInBody |
        ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit, ImVec2(0, 0)))
    {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Offset", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);
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
            if (modelPreview.active && modelPreview.error.empty())
            {
                DrawRow("Model", "string", modelPreview.name);
                DrawRow("meshes", "u32", std::to_string(modelPreview.meshCount));
                DrawRow("vertices", "u32", std::to_string(modelPreview.vertexCount));
                DrawRow("triangles", "u32", std::to_string(modelPreview.triangleCount));
            }
            if (audio)
            {
                DrawRow("formatTag", "u16", Hex(audio->format.formatTag) + " (XMA2)");
                DrawRow("channels", "u16", std::to_string(audio->format.channels));
                DrawRow("samplesPerSec", "u32", std::to_string(audio->format.samplesPerSec));
                DrawRow("avgBytesPerSec", "u32", std::to_string(audio->format.avgBytesPerSec));
                DrawRow("blockAlign", "u16", std::to_string(audio->format.blockAlign));
                DrawRow("bitsPerSample", "u16", std::to_string(audio->format.bitsPerSample));
                DrawRow("numStreams", "u16", std::to_string(audio->format.numStreams));
                DrawRow("samplesEncoded", "u32", std::to_string(audio->format.samplesEncoded));
                DrawRow("bytesPerBlock", "u32", std::to_string(audio->format.bytesPerBlock));
                DrawRow("blockCount", "u16", std::to_string(audio->format.blockCount));
                DrawRow("payload", "bytes", std::to_string(audio->data.size()));
            }
            if (!audioError.empty()) DrawRow("error", "", audioError);
        }
        ImGui::EndTable();
    }

}

void Workspace::AddLog(const std::string& message)
{
    log.push_back(message);
    if (log.size() > 200) log.erase(log.begin());
    logChanged = true;
}

void Workspace::Draw()
{
    session.PollLoad();
    player.Poll(session.status, session.error);
    if (session.status != loggedStatus) { AddLog(session.status); loggedStatus = session.status; }
    if (session.IsLoading() && !loadingLogged) AddLog("Loading package...");
    loadingLogged = session.IsLoading();
    if (session.error != loggedError) {
        if (!session.error.empty()) AddLog("Error: " + session.error);
        loggedError = session.error;
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
    constexpr float gap = 4;
    fieldsWidth = std::clamp(fieldsWidth, 180.0f, std::max(180.0f, width * 0.45f));
    const float leftWidth = std::max(1.0f, width - fieldsWidth - gap);
    splitX = std::clamp(splitX, 100.0f, std::max(100.0f, leftWidth - 120));
    logHeight = std::clamp(logHeight, 60.0f, std::max(60.0f, bodyHeight - 160));
    ImGui::BeginChild("Main panels", ImVec2(leftWidth, bodyHeight));
    ImGui::BeginChild("Upper panels", ImVec2(0, bodyHeight - logHeight - gap));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    ImGui::BeginDisabled(IsBusy());
    ImGui::BeginChild("Chunks", ImVec2(splitX, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding,
        ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextUnformatted("Chunks");
    ImGui::Separator();
    if (session.package)
        for (const CMChunk& chunk : session.package->rootChunks) DrawChunk(chunk, session, true);
    ImGui::EndChild();
    ImGui::SameLine(0, 0);
    ImGui::InvisibleButton("Chunk splitter", ImVec2(gap, ImGui::GetContentRegionAvail().y));
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    if (ImGui::IsItemActive()) splitX += ImGui::GetIO().MouseDelta.x;
    ImGui::SameLine(0, 0);
    RefreshSelection();
    preview.Update(session);
    modelPreview.Update(session);
    ImGui::BeginChild("Resource view", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::TextUnformatted("Resource view");
    ImGui::Separator();
    if (preview.active) preview.Draw();
    else if (modelPreview.active) modelPreview.Draw(&session);
    else if (audio)
    {
        ImGui::TextWrapped("%s", audio->name.c_str());
        ImGui::Text("%u Hz, %u channels", audio->format.samplesPerSec, audio->format.channels);
        if (ImGui::Button("Play sample")) HandleAction(MenuAction::PlayAudio, glfwGetCurrentContext());
        ImGui::SameLine();
        if (ImGui::Button("Extract WAV...")) HandleAction(MenuAction::ExtractAudio, glfwGetCurrentContext());
    }
    else ImGui::TextDisabled("Select a texture, geometry model, or audio sample.");
    ImGui::EndChild();
    ImGui::EndDisabled();
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::InvisibleButton("Log splitter", ImVec2(leftWidth, gap));
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    if (ImGui::IsItemActive()) logHeight -= ImGui::GetIO().MouseDelta.y;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    ImGui::BeginChild("Debug log", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    const bool follow = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();
    ImGui::TextUnformatted("Debug log");
    ImGui::Separator();
    for (const auto& message : log) ImGui::TextWrapped("%s", message.c_str());
    if (logChanged && follow) ImGui::SetScrollHereY(1);
    logChanged = false;
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::SameLine(0, 0);
    ImGui::InvisibleButton("Fields splitter", ImVec2(gap, bodyHeight));
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    if (ImGui::IsItemActive()) fieldsWidth -= ImGui::GetIO().MouseDelta.x;
    ImGui::SameLine(0, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
    ImGui::BeginChild("Fields", ImVec2(0, bodyHeight), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::BeginDisabled(IsBusy());
    DrawFields();
    ImGui::EndDisabled();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::BeginChild("Status", ImVec2(0, statusHeight), ImGuiChildFlags_Borders);
    ImGui::TextUnformatted(session.IsLoading() ? "Loading package..." : session.status.c_str());
    ImGui::EndChild();
    ImGui::End();
    ImGui::PopStyleVar();

    if (IsBusy())
    {
        ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted(session.IsLoading() ? "Loading package..." : "Decoding XMA2 with ffmpeg...");
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
            try {
                ReplaceTexture(session, replacementPath, replacementWidth, replacementHeight);
                AddLog(session.status);
                loggedStatus = session.status;
            }
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