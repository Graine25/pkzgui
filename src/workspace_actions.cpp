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
    if (!session.package || !session.selected) return;
    try
    {
        resource = SelectedResource(*session.package, session.selected);

    }
    catch (const std::exception& ex) { session.error = ex.what(); }
}

void Workspace::UpdateMenus(AppMenu& menu)
{
    const bool ready = !IsBusy() && replacementPath.empty() && !confirmOpen;
    menu.SetEnabled(MenuAction::OpenPackage, ready);
    menu.SetEnabled(MenuAction::Save, ready && session.package != nullptr);
    menu.SetEnabled(MenuAction::SaveAs, ready && session.package != nullptr);
    menu.SetEnabled(MenuAction::ExtractTexture, ready && preview.active && resource && resource->GetResourceType() == kCMResourceType_Texture);
    menu.SetEnabled(MenuAction::ReplaceTexture, ready && preview.active && resource && resource->GetResourceType() == kCMResourceType_Texture);
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
            if (const auto path = PickPackage(window); !path.empty()) { session.BeginLoad(path); }
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
        default: break;
        }
    }
    catch (const std::exception& ex) { session.error = ex.what(); }
}
