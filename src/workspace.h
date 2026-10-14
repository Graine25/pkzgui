#pragma once
#include "package_session.h"
#include "texture_preview.h"
#include "app_menu.h"
#include "audio_player.h"
#include "resource_selection.h"

struct Workspace
{
    float splitX = 340.0f;
    PackageSession session;
    TexturePreview preview;
    AudioPlayer player;
    std::optional<AudioSample> audio;
    std::optional<CMChunkResourceHeader> resource;
    std::string audioError, replacementPath;
    int replacementWidth = 0, replacementHeight = 0;
    bool confirmOpen = false, approvedOpen = false;
    void Draw();
    bool IsBusy() const { return session.IsLoading() || player.IsBusy(); }
    void HandleAction(MenuAction action, GLFWwindow* window);
    void UpdateMenus(AppMenu& menu);

private:
    const CMChunk* detailsSelection = nullptr;
    const PKPackage* detailsPackage = nullptr;
    uint64_t detailsRevision = 0;
    void RefreshSelection();
};
