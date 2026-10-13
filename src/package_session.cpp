#include "package_session.h"
#include <filesystem>
#include <fstream>
#include <iterator>

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
