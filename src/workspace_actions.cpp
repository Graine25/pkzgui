#include "workspace.h"
#include "file_dialog.h"
#include "file_io.h"
#include "texture_actions.h"
#include <GLFW/glfw3.h>

void Workspace::RefreshSelection()
{
    if (detailsSelection == session.selected && detailsPackage == session.package.get() && detailsRevision == session.revision) return;
    detailsSelection = session.selected;
    detailsPackage = session.package.get();
    detailsRevision = session.revision;
    resource.reset();
    audio.reset();
    audioError.clear();
    if (!session.package || !session.selected) return;
    try
    {
        resource = SelectedResource(*session.package, session.selected);
        const uint32_t id = session.selected->GetMaskedID();
        if ((resource && resource->GetResourceType() == kCMResourceType_AudioSample) || id == Audio_SampleData || id == CutScene_Audio_SampleData)
            audio = SelectedAudio(session);
    }
    catch (const std::exception& ex) { audioError = ex.what(); }
}

void Workspace::UpdateMenus(AppMenu& menu)
{
    const bool ready = !IsBusy() && replacementPath.empty() && !confirmOpen;
    menu.SetEnabled(MenuAction::OpenPackage, ready);
    menu.SetEnabled(MenuAction::Save, ready && session.package != nullptr);
    menu.SetEnabled(MenuAction::SaveAs, ready && session.package != nullptr);
    menu.SetEnabled(MenuAction::ExtractTexture, ready && preview.active && resource && resource->GetResourceType() == kCMResourceType_Texture);
    menu.SetEnabled(MenuAction::ReplaceTexture, ready && preview.active && resource && resource->GetResourceType() == kCMResourceType_Texture);
    menu.SetEnabled(MenuAction::ExtractAudio, ready && audio.has_value());
    menu.SetEnabled(MenuAction::PlayAudio, ready && audio.has_value());
}

void Workspace::HandleAction(MenuAction action, GLFWwindow* window)
{
    if (action == MenuAction::Exit) { glfwSetWindowShouldClose(window, GLFW_TRUE); return; }
    if (IsBusy() || !replacementPath.empty() || confirmOpen) return;
    try
    {
        switch (action)
        {
        case MenuAction::OpenPackage:
            if (session.dirty && !approvedOpen) { confirmOpen = true; return; }
            approvedOpen = false;
            if (const auto path = PickPackage(window); !path.empty()) { player.Stop(); session.BeginLoad(path); }
            break;
        case MenuAction::Save:
        case MenuAction::SaveAs:
            if (session.package)
            {
                std::string path = session.path;
                if (action == MenuAction::SaveAs || path.empty())
                    path = PickFile(window, "Package files", "pkz,pak", true, std::filesystem::u8path(path).filename().u8string());
                if (!path.empty()) session.Save(path);
            }
            break;
        case MenuAction::ExtractTexture:
        {
            const auto texture = SelectedTexture(session);
            const auto path = PickFile(window, "PNG", "png", true, SanitizeFileName(texture.name) + ".png");
            if (!path.empty()) { ExtractTexture(session, path); session.status = "Extracted " + path; }
            break;
        }
        case MenuAction::ReplaceTexture:
        {
            const auto texture = SelectedTexture(session);
            const auto path = PickFile(window, "PNG", "png", false);
            if (!path.empty()) { replacementPath = path; replacementWidth = texture.desc.width; replacementHeight = texture.desc.height; }
            break;
        }
        case MenuAction::ExtractAudio:
        case MenuAction::PlayAudio:
        {
            const auto sample = SelectedAudio(session);
            std::string path;
            if (action == MenuAction::ExtractAudio)
            {
                path = PickFile(window, "WAV", "wav", true, SanitizeFileName(sample.name) + ".wav");
                if (path.empty()) break;
            }
            player.Begin(sample, path);
            session.status = "Decoding XMA2 with ffmpeg...";
            break;
        }
        default: break;
        }
    }
    catch (const std::exception& ex) { session.error = ex.what(); }
}
