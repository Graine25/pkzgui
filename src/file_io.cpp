#include "file_io.h"
#include <fstream>
#include <iterator>
#include <stdexcept>

std::vector<uint8_t> ReadFile(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Cannot open file: " + path.u8string());
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)), {});
    if (input.bad())
        throw std::runtime_error("Cannot read file: " + path.u8string());
    return bytes;
}

void WriteFile(const std::filesystem::path& path, const std::vector<uint8_t>& bytes)
{
    std::ofstream output(path, std::ios::binary);
    if (!output)
        throw std::runtime_error("Cannot open file for writing: " + path.u8string());
    output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    output.close();
    if (!output)
        throw std::runtime_error("Failed writing file: " + path.u8string());
}

std::string SanitizeFileName(std::string name)
{
    if (name.empty()) name = "sample";
    for (char& c : name)
        if (std::string("/\\:*?\"<>|").find(c) != std::string::npos || static_cast<unsigned char>(c) < 32)
            c = '_';
    return name;
}
