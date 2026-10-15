#pragma once
#include <filesystem>
#include <vector>
#include <cstdint>
#include <string>

std::vector<uint8_t> ReadFile(const std::filesystem::path& path);
void WriteFile(const std::filesystem::path& path, const std::vector<uint8_t>& bytes);
std::string SanitizeFileName(std::string name);

std::filesystem::path DebugLogPath();
void AppendDebugLog(const std::string& message);
