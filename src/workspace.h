#pragma once
#include "package_session.h"

struct Workspace
{
    float splitX = 340.0f;
    PackageSession session;
    void Draw();
};
