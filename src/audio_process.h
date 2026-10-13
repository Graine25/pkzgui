#pragma once
#include <filesystem>
#include <vector>
#include <string>

struct TemporaryDirectory
{
    std::filesystem::path path;
    TemporaryDirectory();
    ~TemporaryDirectory();
};

std::filesystem::path FindFfmpeg();
void RunFfmpeg(const std::filesystem::path& input, const std::filesystem::path& output);
