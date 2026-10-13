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
    std::string path;
    bool dirty = false;
    uint64_t revision = 0;

    bool Load(const std::string& path);
    bool Save(const std::string& destination);
    bool BeginLoad(const std::string& path);
    void PollLoad();
    bool IsLoading() const { return pending.valid(); }

private:
    std::vector<uint8_t> lastSavedBytes;
    std::future<std::unique_ptr<PackageSession>> pending;
};
