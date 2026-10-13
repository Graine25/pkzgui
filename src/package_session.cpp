#include "package_session.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <chrono>

bool PackageSession::BeginLoad(const std::string& path)
{
    if (IsLoading())
        return false;
    try
    {
        pending = std::async(std::launch::async, [path] {
            auto result = std::make_unique<PackageSession>();
            result->Load(path);
            return result;
        });
        error.clear();
        return true;
    }
    catch (const std::exception& ex)
    {
        error = ex.what();
        return false;
    }
}

void PackageSession::PollLoad()
{
    if (!IsLoading() || pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        return;
    try
    {
        auto result = pending.get();
        if (result->package)
        {
            package = std::move(result->package);
            selected = &package->rootChunks.front();
            status = std::move(result->status);
        }
        error = std::move(result->error);
    }
    catch (const std::exception& ex)
    {
        error = ex.what();
    }
}

bool PackageSession::Load(const std::string& path)
{
    try
    {
        std::ifstream file(std::filesystem::u8path(path), std::ios::binary);
        if (!file)
            throw std::runtime_error("Cannot open package: " + path);
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)), {});
        if (file.bad())
            throw std::runtime_error("Cannot read package: " + path);
        auto candidate = std::make_unique<PKPackage>();
        try
        {
            candidate->ReadFromMemory(bytes);
            if (candidate->rootChunks.empty() || candidate->rootChunks.front().GetMaskedID() == 0)
                throw std::runtime_error("No valid root chunk");
        }
        catch (const std::exception&)
        {
            candidate = std::make_unique<PKPackage>();
            candidate->isLittleEndian = true;
            candidate->ReadFromMemory(bytes);
        }
        if (candidate->rootChunks.empty())
            throw std::runtime_error("Package contains no chunks: " + path);
        std::string loadedStatus = path + (candidate->wasCompressed ? " (compressed, " : " (raw, ") +
            (candidate->isLittleEndian ? "little-endian)" : "big-endian)");
        package = std::move(candidate);
        selected = &package->rootChunks.front();
        status = std::move(loadedStatus);
        error.clear();
        return true;
    }
    catch (const std::exception& ex)
    {
        error = ex.what();
        return false;
    }
}
