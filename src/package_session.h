#pragma once
#include "Package/PKPackage.h"
#include <memory>
#include <future>
#include <string>

struct PackageSession
{
    std::unique_ptr<PKPackage> package;
    const CMChunk* selected = nullptr;
    std::string status = "No package loaded.";
    std::string error;

    bool Load(const std::string& path);
    bool BeginLoad(const std::string& path);
    void PollLoad();
    bool IsLoading() const { return pending.valid(); }

private:
    std::future<std::unique_ptr<PackageSession>> pending;
};
