#pragma once
#include "Package/PKPackage.h"
#include <memory>
#include <string>

struct PackageSession
{
    std::unique_ptr<PKPackage> package;
    const CMChunk* selected = nullptr;
    std::string status = "No package loaded.";
    std::string error;

    bool Load(const std::string& path);
};
