#pragma once
#include "package/package_session.h"
#include "texture/texture_preview.h"
#include "model/model_preview.h"
#include "ui/app_menu.h"
#include "audio/audio_player.h"
#include "package/resource_selection.h"

struct Workspace
{
    float splitX = 280.0f;
    float fieldsWidth = 340.0f, logHeight = 200.0f, modelActionsHeight = 280.0f;
    PackageSession session;
    TexturePreview preview;
    ModelPreview modelPreview;
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
    void DrawFields();
    void AddLog(const std::string& message);
    std::vector<std::string> log;
    std::string loggedStatus, loggedError;
    bool loadingLogged = false;
    bool logChanged = false;
};