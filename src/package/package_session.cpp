#include "package/package_session.h"
#include "platform/file_io.h"
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
            path = std::move(result->path);
            lastSavedBytes = std::move(result->lastSavedBytes);
            dirty = false;
            ++revision;
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
        auto snapshot = candidate->Serialize();
        package = std::move(candidate);
        lastSavedBytes = std::move(snapshot);
        this->path = path;
        dirty = false;
        ++revision;
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


bool PackageSession::Save(const std::string& destination)
{
    if (!package || IsLoading()) return false;
    try
    {
        for (CMChunk& root : package->rootChunks) root.ComputeLength();
        auto bytes = package->Serialize();
        const auto outputPath = std::filesystem::u8path(destination);
        std::string result;
        if (package->wasCompressed)
        {
            BUCompress::Options options;
            options.sectorSize = package->compressHeader.uiSectorSize ? package->compressHeader.uiSectorSize : 0x10000;
            options.bigEndian32 = true;
            WriteFile(outputPath, BUCompress::Compress(bytes, options));
            result = "Saved (recompressed full file)";
        }
        else
        {
            std::error_code ec;
            const bool same = std::filesystem::equivalent(outputPath, std::filesystem::u8path(path), ec);
            std::fstream output;
            if (same && bytes.size() == lastSavedBytes.size() && std::filesystem::file_size(outputPath) == bytes.size())
                output.open(outputPath, std::ios::binary | std::ios::in | std::ios::out);
            if (output.is_open())
            {
                size_t changed = 0;
                for (size_t i = 0; i < bytes.size();)
                {
                    if (bytes[i] == lastSavedBytes[i]) { ++i; continue; }
                    const size_t start = i;
                    while (i < bytes.size() && bytes[i] != lastSavedBytes[i]) ++i;
                    output.seekp(start);
                    output.write(reinterpret_cast<const char*>(bytes.data() + start), i - start);
                    changed += i - start;
                }
                output.close();
                if (!output) throw std::runtime_error("Write failed.");
                result = "Saved (patched " + std::to_string(changed) + " differing byte(s))";
            }
            else
            {
                WriteFile(outputPath, bytes);
                result = "Saved (full rewrite)";
            }
        }
        lastSavedBytes = std::move(bytes);
        path = destination;
        dirty = false;
        status = result;
        error.clear();
        return true;
    }
    catch (const std::exception& ex)
    {
        error = ex.what();
        return false;
    }
}
