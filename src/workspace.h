#pragma once
#include "package_session.h"
#include "texture_preview.h"

struct Workspace
{
    float splitX = 340.0f;
    PackageSession session;
    TexturePreview preview;
    void Draw();
};
